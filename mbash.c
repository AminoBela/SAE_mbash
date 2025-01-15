#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#define MAXLI 2048

#define MAX_TOKENS 128
#define MAX_TOKEN_LENGTH 256

typedef enum {
    START,
    COMMAND,
    ARGUMENT,
    OPERATOR,
    END,
    ERROR
} State;

typedef enum {
    NONE,
    SEQUENCE, // ;
    AND,      // &&
    OR        // ||
} Operator;

typedef struct {
    char *command;
    char *args[MAX_TOKENS];
    Operator next_operator;
} ParsedCommand;

void parse_line(const char *line, ParsedCommand commands[], int *num_commands);
int execute_command(ParsedCommand *cmd);
void history();
void save_history(ParsedCommand *cmd);
void clear_history();

int main() {
    char line[1024];
    ParsedCommand commands[MAX_TOKENS];
    int num_commands;

    while (1) {
        printf("> ");
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break; // Fin de fichier (Ctrl+D)
        }
        line[strcspn(line, "\n")] = '\0'; // Supprime le \n final

        // Réinitialisation
        num_commands = 0;
        memset(commands, 0, sizeof(commands));

        // Analyse de la ligne
        parse_line(line, commands, &num_commands);

        // Exécution des commandes
        int last_status = 0;
        for (int i = 0; i < num_commands; i++) {
            if (i > 0) {
                // Gérer les opérateurs logiques
                if (commands[i - 1].next_operator == AND && last_status != 0) {
                    continue; // Ne pas exécuter si la précédente a échoué
                }
                if (commands[i - 1].next_operator == OR && last_status == 0) {
                    continue; // Ne pas exécuter si la précédente a réussi
                }
            }
            last_status = execute_command(&commands[i]);
        }
    }

    return 0;
}

void parse_line(const char *line, ParsedCommand commands[], int *num_commands) {
    State state = START;
    char token[MAX_TOKEN_LENGTH];
    int token_idx = 0;
    int cmd_idx = -1;
    int arg_idx = 0; // Déclaré ici pour être utilisé pour les arguments

    for (int i = 0; i <= strlen(line); i++) {
        char c = line[i];

        switch (state) {
        case START:
            if (!isspace(c)) {
                state = COMMAND;
                token[token_idx++] = c;
                cmd_idx++;
                arg_idx = 0; // Réinitialiser l'index des arguments pour chaque commande
                commands[cmd_idx].next_operator = NONE; // Par défaut
            }
            break;

        case COMMAND:
            if (isspace(c) || c == '\0') {
                token[token_idx] = '\0';
                commands[cmd_idx].command = strdup(token);
                commands[cmd_idx].args[arg_idx++] = strdup(token);
                token_idx = 0;
                state = ARGUMENT;
            } else if (c == ';' || c == '&' || c == '|') {
                token[token_idx] = '\0';
                commands[cmd_idx].command = strdup(token);
                commands[cmd_idx].args[arg_idx++] = strdup(token);
                token_idx = 0;
                state = OPERATOR;
                i--; // Réanalyser l'opérateur
            } else {
                token[token_idx++] = c;
            }
            break;

        case ARGUMENT:
            if (isspace(c) || c == '\0') {
                if (token_idx > 0) {
                    token[token_idx] = '\0';
                    commands[cmd_idx].args[arg_idx++] = strdup(token);
                    token_idx = 0;
                }
                if (c == '\0') {
                    commands[cmd_idx].args[arg_idx] = NULL; // Terminer le tableau des arguments
                }
            } else if (c == ';' || c == '&' || c == '|') {
                commands[cmd_idx].args[arg_idx] = NULL; // Terminer avant de passer à un opérateur
                state = OPERATOR;
                i--; // Réanalyser l'opérateur
            } else {
                token[token_idx++] = c;
            }
            break;

        case OPERATOR:
        if (c == ';') {
            commands[cmd_idx].next_operator = SEQUENCE;
            commands[cmd_idx].args[arg_idx] = NULL; // Terminer le tableau des arguments
            state = START;
        } else if (c == '&' && line[i + 1] == '&') {
            commands[cmd_idx].next_operator = AND;
            commands[cmd_idx].args[arg_idx] = NULL; // Terminer le tableau des arguments
            i++; // Sauter le deuxième &
            state = START;
        } else if (c == '|' && line[i + 1] == '|') {
            commands[cmd_idx].next_operator = OR;
            commands[cmd_idx].args[arg_idx] = NULL; // Terminer le tableau des arguments
            i++; // Sauter le deuxième |
            state = START;
        }
        break;


        default:
            break;
        }
    }

    *num_commands = cmd_idx + 1;
}


int execute_command(ParsedCommand *cmd) {
    if (!cmd->command) {
        return -1; // Commande invalide
    }

    save_history(cmd);

    // Gestion des commandes internes
    if (strcmp(cmd->command, "cd") == 0) {
        if (cmd->args[1] == NULL) {
            fprintf(stderr, "Erreur : chemin non spécifié pour cd\n");
            return -1;
        }
        if (chdir(cmd->args[1]) != 0) {
            perror("Erreur lors du changement de répertoire");
            return -1;
        }
        return 0;
    }

    if (strcmp(cmd->command, "history") == 0) {
        if (cmd->args[1] == NULL) {
            history();
        }
        if (cmd->args[1] != NULL) {
            if (strcmp(cmd->args[1], "-c") == 0) {
                clear_history();
            }
        }
    }

    if (strcmp(cmd->command, "exit") == 0) {
        exit(0); // Quitter le shell
    }

    // Commandes externes
    pid_t pid = fork();
    if (pid == 0) {
        // Processus enfant
        execvp(cmd->command, cmd->args);
        perror("Erreur lors de l'exécution de la commande");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Processus parent
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status); // Retourne le code de sortie
    } else {
        perror("Erreur de fork");
        return -1;

  }

}

void history() {

    // Ouvrir le fichier historique.txt en mode lecture
    FILE* file = fopen("history.txt", "r");
    if (file == NULL) {
        perror("Erreur d'ouverture du fichier");
        return;
    }

    // Lire et afficher chaque ligne du fichier
    char line[MAXLI];
    while (fgets(line, MAXLI, file) != NULL) {
        printf("- %s", line); // Afficher chaque ligne lue
    }

    // Fermer le fichier
    fclose(file);

}

void save_history(ParsedCommand *cmd) {
    // Ouvrir le fichier en mode ajout
    FILE* file = fopen("history.txt", "a");

    // Si l'utilisateur n'as pas juste appuyé sur entrer
    if (strcmp("\n", cmd->command) != 0 && strcmp("", cmd->command) != 0) {
        char* strArgs = "";
        for (int i = 1; cmd->args[i] != NULL; i++) {
            strArgs = strcat(strArgs, cmd->args[i]);
            strArgs = strcat(strArgs, " ");
        }
        // Écrire la commande dans le fichier
        fprintf(file, "%s %s\n", cmd->command, strArgs);
    }

    fclose(file);

}

void clear_history() {

    FILE* file = fopen("history.txt", "w");
    if (file != NULL) {
        remove("history.txt");
        history();
    }

    fclose(file);

}