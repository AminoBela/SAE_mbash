#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h> // pour waitpid et WEXITSTATUS
#include <ctype.h> // pour isspace
#define MAXLI 2048 // Taille maximale d'une ligne

#define MAX_TOKENS 128 // Nombre maximal de commandes
#define MAX_TOKEN_LENGTH 256 // Taille maximale d'une commande

/**
 *  États possibles pour l'analyseur de ligne de commande
 *  START: début de la ligne
 *  COMMAND: analyse du nom de la commande
 *  ARGUMENT: analyse des arguments de la commande
 *  OPERATOR: analyse de l'opérateur logique
 *  END: fin de la ligne
 *  ERROR: erreur de syntaxe
 */
typedef enum {
    START,
    COMMAND,
    ARGUMENT,
    OPERATOR,
    END,
    ERROR
} State;

/**
 *  Opérateurs logiques possibles
 *  NONE: aucun opérateur
 *  SEQUENCE: ; (séquence)
 *  AND: && (et logique)
 *  OR: || (ou logique)
 */
typedef enum {
    NONE,
    SEQUENCE, // ;
    AND,      // &&
    OR        // ||
} Operator;

/**
 *  Structure pour stocker une commande parsée (nom de la commande et ses arguments)
 *  command: nom de la commande
 *  args: tableau des arguments de la commande
 *  next_operator: opérateur logique suivant
 */
typedef struct {
    char *command;
    char *args[MAX_TOKENS];
    Operator next_operator;
} ParsedCommand;

/**
 * Déclaration des fonctions
 */
void parse_line(const char *line, ParsedCommand commands[], int *num_commands);
int execute_command(ParsedCommand *cmd);
void history();
void save_history(ParsedCommand *cmd);
void clear_history();

/**
 * Fonction principale
 */
int main() {

    char line[1024]; // Ligne de commande
    ParsedCommand commands[MAX_TOKENS]; // Commandes parsées
    int num_commands; // Nombre de commandes

    /**
     * Boucle principale, lit une ligne de commande à la fois
     */
    while (1) {
        printf(">");

        // Lire une ligne de commande
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

        /**
         * Exécute chaque commande, en tenant compte des opérateurs logiques
         */
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

/**
 * Fonction pour séparer une ligne de commande en commandes et arguments
 * @param line
 * @param commands
 * @param num_commands
 */
void parse_line(const char *line, ParsedCommand commands[], int *num_commands) {
    State state = START; // État initial
    char token[MAX_TOKEN_LENGTH]; // Token actuel pour stocker commande/argument
    int token_idx = 0; // Index dans le token
    int cmd_idx = -1; // Index de la commande actuelle
    int arg_idx = 0; // Index des arguments de la commande actuelle

    for (int i = 0; i <= strlen(line); i++) {
        char c = line[i];

        switch (state) {
        case START:
            if (!isspace(c)) { // Si on trouve une commande ou un argument
                state = COMMAND;
                token[token_idx++] = c; // Commencer à stocker le token
                cmd_idx++; // Nouvelle commande
                arg_idx = 0; // Réinitialiser l'index des arguments
                commands[cmd_idx].next_operator = NONE; // Par défaut
            }
            break;

        case COMMAND:
            if (isspace(c) || c == '\0' || c == ';' || c == '&' || c == '|') {
                if (token_idx > 0) { // Fin du nom de la commande
                    token[token_idx] = '\0'; // Terminer le token
                    commands[cmd_idx].command = strdup(token); // Enregistrer la commande
                    commands[cmd_idx].args[arg_idx++] = strdup(token); // Ajouter comme premier argument
                    token_idx = 0; // Réinitialiser l'index du token
                }

                // Si c'est un opérateur ou la fin, terminer la commande actuelle
                if (c == ';') {
                    commands[cmd_idx].next_operator = SEQUENCE;
                    commands[cmd_idx].args[arg_idx] = NULL; // Terminer les arguments
                    state = START; // Recommencer pour une nouvelle commande
                } else if (c == '&' && line[i + 1] == '&') {
                    commands[cmd_idx].next_operator = AND;
                    commands[cmd_idx].args[arg_idx] = NULL;
                    i++; // Sauter le deuxième &
                    state = START;
                } else if (c == '|' && line[i + 1] == '|') {
                    commands[cmd_idx].next_operator = OR;
                    commands[cmd_idx].args[arg_idx] = NULL;
                    i++; // Sauter le deuxième |
                    state = START;
                } else if (c == '\0') {
                    commands[cmd_idx].args[arg_idx] = NULL; // Terminer les arguments
                } else { // Passer aux arguments
                    state = ARGUMENT;
                }
            } else {
                token[token_idx++] = c; // Continuer à enregistrer le nom de la commande
            }
            break;

        case ARGUMENT:
            if (isspace(c) || c == '\0' || c == ';' || c == '&' || c == '|') {
                if (token_idx > 0) { // Fin de l'argument
                    token[token_idx] = '\0'; // Terminer le token
                    commands[cmd_idx].args[arg_idx++] = strdup(token); // Ajouter l'argument
                    token_idx = 0; // Réinitialiser l'index du token
                }

                if (c == ';') {
                    commands[cmd_idx].next_operator = SEQUENCE;
                    commands[cmd_idx].args[arg_idx] = NULL; // Terminer les arguments
                    state = START; // Nouvelle commande
                } else if (c == '&' && line[i + 1] == '&') {
                    commands[cmd_idx].next_operator = AND;
                    commands[cmd_idx].args[arg_idx] = NULL;
                    i++;
                    state = START;
                } else if (c == '|' && line[i + 1] == '|') {
                    commands[cmd_idx].next_operator = OR;
                    commands[cmd_idx].args[arg_idx] = NULL;
                    i++;
                    state = START;
                } else if (c == '\0') {
                    commands[cmd_idx].args[arg_idx] = NULL; // Fin de la ligne
                }
            } else {
                token[token_idx++] = c; // Continuer à enregistrer l'argument
            }
            break;

        default:
            break;
        }
    }

    // Supprimer les commandes vides
    if (cmd_idx >= 0 && commands[cmd_idx].command == NULL) {
        cmd_idx--;
    }

    *num_commands = cmd_idx + 1; // Mettre à jour le nombre total de commandes
}




/**
 * Fonction pour exécuter une commande
 * @param cmd
 * @return
 */
int execute_command(ParsedCommand *cmd) {

    // Vérifier si la commande est valide
    if (!cmd->command) {
        return -1; // Commande invalide
    }

    // Sauvegarde de l'historique pour la commande history (voir cette fonction pour en savoir plus)
    save_history(cmd);

    // Gestion des commandes internes
    if (strcmp(cmd->command, "cd") == 0) {
        // Changer de répertoire
        if (cmd->args[1] == NULL) {
            fprintf(stderr, "Erreur : chemin non spécifié pour cd\n");
            return -1;
        }
        // Changer de répertoire, avec gestion des erreurs
        if (chdir(cmd->args[1]) != 0) {
            perror("Erreur lors du changement de répertoire");
            return -1;
        }
        return 0;
    }

    // Commande history (voir cette fonction pour en savoir plus)
    if (strcmp(cmd->command, "history") == 0) {
        // Afficher l'historique
        if (cmd->args[1] == NULL) {
            history();
        }
        // Effacer l'historique dans le cas de la commande history -c
        else if (strcmp(cmd->args[1], "-c") == 0) {
            clear_history();
        }
        return 0;
    }

    // Commande exit
    if (strcmp(cmd->command, "exit") == 0) {
        exit(0); // Quitter le shell
    }

    // Commande externe avec fork et execvp (pour info, fork crée un processus enfant)
    // et execvp remplace le processus enfant par un nouveau processus
    pid_t pid = fork();
    // Si pid == 0, c'est le processus enfant qui exécute la commande
    if (pid == 0) {
        // Processus enfant
        execvp(cmd->command, cmd->args);
        perror("Erreur lors de l'exécution de la commande");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Processus parent
        int status;
        waitpid(pid, &status, 0); // Attendre la fin du processus enfant
        return WEXITSTATUS(status); // Retourne le code de sortie
    } else {
        perror("Erreur de fork");
        return -1;
  }
}

/**
 * Fonction pour afficher l'historique des commandes
 */
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

/**
 * Fonction pour sauvegarder l'historique des commandes
 * @param cmd
 */
void save_history(ParsedCommand *cmd) {
    // Ouvrir le fichier en mode ajout
    FILE* file = fopen("history.txt", "a");
    if (file == NULL) {
        perror("Erreur d'ouverture du fichier");
        return;
    }

    // Si l'utilisateur n'as pas juste appuyé sur entrer
    if (strcmp("\n", cmd->command) != 0 && strcmp("", cmd->command) != 0) {
        char* strArgs = "";

        // Écrire la commande dans le fichier
        fprintf(file, "%s %s\n", cmd->command, strArgs);
    }

    fclose(file);

}

/**
 * Fonction pour effacer l'historique des commandes
 */
void clear_history() {
    FILE* file = fopen("history.txt", "w");
    if (file == NULL) {
        perror("Erreur d'ouverture du fichier");
        return;
    }

    fclose(file);
    printf("Historique effacé.\n");
}

