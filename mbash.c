/**
 *  MBASH - Mini shell en C
 *  Auteur : Amin Belalia et Clement De Wasch
 */

/****************************************************
* Inclusions
*****************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h> // pour waitpid et WEXITSTATUS
#include <ctype.h> // pour isspace
#include <readline/readline.h>
#include <readline/history.h>

/****************************************************
 * Constantes
 ****************************************************/

#define MAXLI 2048 // Taille maximale d'une ligne
#define PATHMAX 4096
#define MAX_LINE_LENGTH 1024 // Taille maximale d'une ligne
#define MAX_TOKENS 128 // Nombre maximal de commandes
#define MAX_TOKEN_LENGTH 256 // Taille maximale d'une commande

/****************************************************
 * Enumérations et structures
 ****************************************************/

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
    STATE_QUOTE,
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
    int background;
} ParsedCommand;

/**
 * Déclaration des fonctions
 */
void parse_line(const char *line, ParsedCommand commands[], int *num_commands);
int execute_command(ParsedCommand *cmd);
void history();
void save_history(ParsedCommand *cmd);
void clear_history();
void set_environment_variable(char *name, char *value);
void unset_environment_variable(char *name);
void print_environment_variables();
char* expand_variable(const char *token);
int handle_up_arrow(int count, int key);
void expand_arguments(ParsedCommand *cmd);
char* read_line_from_file(const char* filename, int target_line);

int handle_up_arrow(int count, int key) {
    int i = 1;
    //for (i = 1; i <= ; i++) {
    char* line = read_line_from_file("history.txt", i);
    return 0; // Retourner 0 pour continuer
}
/****************************************************
 * Point d'entrée du programme
 ****************************************************/

/**
 * Fonction principale
 */
int main() {

    char *input; // iNput de l'utilisateur
    char line[1024]; // Ligne de commande
    ParsedCommand commands[MAX_TOKENS]; // Commandes parsées
    int num_commands; // Nombre de commandes
    char cwd[PATH_MAX]; // Répertoire courant

    signal(SIGINT, SIG_IGN); // Ignorer le signal SIGINT (Ctrl+C)

    rl_bind_keyseq("\\e[A", handle_up_arrow); // Lier la flèche Haut à la fonction handle_up_arrow

    /**
     * Boucle principale, lit une ligne de commande à la fois
     */
    while (1) {

        //printf("%s > ", getcwd(cwd, sizeof(cwd)));
        //printf("%s >"/*, getcwd(cwd, sizeof(cwd))*/);
        input = readline("> ");

        //Lire une ligne de commande
        if (input == NULL) {
            break;
        }

        /*Lire une ligne de commande
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break; // Fin de fichier (Ctrl+D)
        }*/

        line[strcspn(line, "\n")] = '\0'; // Supprime le \n final

        // Réinitialisation
        num_commands = 0;
        memset(commands, 0, sizeof(commands));

        // Analyse de la ligne
        parse_line(input, commands, &num_commands);

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
    free(input);
    return 0;
}

/****************************************************
 * Fonctions pour l'analyse de la ligne de commande
 ****************************************************/

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

    /**
    * Parcourir chaque caractère de la ligne de commande
    */
    for (int i = 0; i <= strlen(line); i++) {
        char c = line[i];

        /**
        * Gestion des états de l'analyseur de ligne de commande
        */
        switch (state) {

            /**
            * État initial : début de la ligne
            */
            case START:
                if (!isspace(c)) { // Début d'une commande ou argument
                    if (c == '"') { // Si on trouve un guillemet ouvrant
                        state = STATE_QUOTE;
                        token_idx = 0;
                    } else if (c == ';') {
                        continue;
                    } else {
                        state = COMMAND;
                        token[token_idx++] = c; // Commencer à stocker le token
                        cmd_idx++;
                        arg_idx = 0;
                        commands[cmd_idx].next_operator = NONE; // Aucun opérateur par défaut
                    }
                }
                break;

            /*
            * État de la commande : analyse du nom de la commande
            */
            case COMMAND:
                // Si on trouve un guiillemet ouvrant, on passe à l'état STATE_QUOTE
                if (c == '"') {
                  state = STATE_QUOTE;
                  token_idx = 0;

                  } else if (isspace(c) || c == '\0' || c == ';' || c == '&' || c == '|') {
                    if (token_idx > 0) {
                        token[token_idx] = '\0';
                        commands[cmd_idx].command = strdup(token);
                        commands[cmd_idx].args[arg_idx++] = strdup(token);
                        token_idx = 0;
                    }
                    if (c == '&' && line[i + 1] != '&') {
                        commands[cmd_idx].background = 1;
                        commands[cmd_idx].args[arg_idx] = NULL;
                        state = START;
                    } else if (c == ';') {
                        commands[cmd_idx].next_operator = SEQUENCE;
                        commands[cmd_idx].args[arg_idx] = NULL;
                        state = START;
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
                        commands[cmd_idx].args[arg_idx] = NULL;
                    } else {
                        state = ARGUMENT;
                    }
                } else {
                    token[token_idx++] = c;
                }
                break;

            /**
            * État de l'argument : analyse des arguments de la commande
            */
            case ARGUMENT:
             if (isspace(c)) {
               continue;
             } else if (c == '"') {
                 state = STATE_QUOTE;
                 token_idx = 0;
	     } else if (c == '$') {
	       token[token_idx++] = c; // Ajoute le '$' pour expansion
	       state = ARGUMENT;
             } else if (c == '\0' || c == ';' || c == '&' || c == '|') {
                 if (token_idx > 0) {
                    token[token_idx] = '\0';
                    commands[cmd_idx].args[arg_idx++] = strdup(token);
                    token_idx = 0;
                 }
                 if (c == ';') {
                     commands[cmd_idx].next_operator = SEQUENCE;
                     commands[cmd_idx].args[arg_idx] = NULL;
                     state = START;
                 } else if (c == '&' && line[i + 1] == '&') {
                    commands[cmd_idx].next_operator = AND;
                    commands[cmd_idx].args[arg_idx] = NULL;
                    i++;
                    state = START;
                 } else if (c == '&' && line[i + 1] != '&') {
                     commands[cmd_idx].background = 1;
                     commands[cmd_idx].args[arg_idx] = NULL;
                     state = START;
                 } else if (c == '|' && line[i + 1] == '|') {
                     commands[cmd_idx].next_operator = OR;
                     commands[cmd_idx].args[arg_idx] = NULL;
                    i++;
                    state = START;
                 } else if (c == '\0') {
                    commands[cmd_idx].args[arg_idx] = NULL;
                 }
             } else {
                 token[token_idx++] = c;
             }
             break;

            /**
            * État de guillemet : analyse des guillemets
            */
            case STATE_QUOTE:
                if (c == '"') {
                    token[token_idx] = '\0';
                    commands[cmd_idx].args[arg_idx++] = strdup(token);
                    token_idx = 0;
                    state = ARGUMENT;
                } else if (c == '\0') {
                    fprintf(stderr, "Erreur : guillemets non fermés.\n");
                    *num_commands = 0;
                    return;
                } else {
                    token[token_idx++] = c;
                }
                break;

            default:
                break;
        }
    }
    if (cmd_idx >= 0 && commands[cmd_idx].command == NULL) {
        cmd_idx--; // Supprimer les commandes vides
    }
    *num_commands = cmd_idx + 1;
}

/****************************************************
 * Fonctions pour l'exécution des commandes
 ****************************************************/

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

    expand_arguments(cmd);


    // Commande interne pour définir une variable d'environnement
    if (strcmp(cmd->command, "export") == 0) {
        // Définir une variable d'environnement
        if (cmd->args[1] == NULL) {
            fprintf(stderr, "Erreur : nom de la variable non spécifié pour export\n");
            return -1;
        } else {
          char *name = strtok(cmd->args[1], "=");
            char *value = strtok(NULL, "=");
            if (name && value) {
                set_environment_variable(name, value);
            } else {
                fprintf(stderr, "Erreur : nom ou valeur non spécifié pour export\n");
                return -1;
            }
          }
        return 0;
    }

    // Commande interne pour supprimer une variable d'environnement
    if (strcmp(cmd->command, "unset") == 0) {
        // Supprimer une variable d'environnement
        if (cmd->args[1] == NULL) {
            fprintf(stderr, "Erreur : nom de la variable non spécifié pour unset\n");
            return -1;
        }
        unset_environment_variable(cmd->args[1]);
        return 0;
    }

    // Commande interne pour afficher les variables d'environnement
    if (strcmp(cmd->command, "env") == 0) {
        // Afficher les variables d'environnement
        print_environment_variables();
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
      if (cmd->background) {
            // Ignorer le signal SIGINT (Ctrl+C) pour les commandes en arrière-plan
            signal(SIGINT, SIG_IGN);
        }
        // Processus enfant
        execvp(cmd->command, cmd->args);
        perror("Erreur lors de l'exécution de la commande");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Processus parent
        if (!cmd->background) {
            // Attendre la fin si la commande n'est pas en arrière-plan
            int status;
            waitpid(pid, &status, 0);
            return WEXITSTATUS(status);
        } else {
            // Afficher un message pour les commandes en arrière-plan
            printf("[PID %d] Commande exécutée en arrière-plan\n", pid);
            return 0; // Considéré comme réussi pour le parent
        }
    } else {
        perror("Erreur de fork");
        return -1;
  }
}

/****************************************************
 * Fonctions pour les commandes internes
 ****************************************************/

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

    // Si l'utilisateur n'a pas juste appuyé sur Entrée
    if (strcmp("\n", cmd->command) != 0 && strcmp("", cmd->command) != 0) {
        // Allouer une mémoire initiale pour strArgs
        size_t total_length = 1; // Pour le caractère nul
        char* strArgs = malloc(total_length);

        strArgs[0] = '\0'; // Initialiser comme chaîne vide

        // Construire la chaîne strArgs avec les arguments
        for (int i = 1; cmd->args[i] != NULL; i++) {

            // Ajouter l'argument et un espace
            total_length += strlen(cmd->args[i]) + 1;
            char* temp = realloc(strArgs, total_length);
            if (temp == NULL) {
                perror("Erreur de réallocation de mémoire");
                free(strArgs);
                fclose(file);
                return;
            }
            strArgs = temp;
            strcat(strArgs, cmd->args[i]);
            strcat(strArgs, " ");
        }

        // Écrire la commande dans le fichier
        fprintf(file, "%s %s\n", cmd->command, strArgs);

        // Libérer la mémoire allouée
        free(strArgs);
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

/**
 * Fonction pour définir une variable d'environnement
 * @param name
 * @param value
 */
void set_environment_variable(char *name, char *value) {
    if (setenv(name, value, 1) != 0) {
        perror("Erreur lors de la définition de la variable d'environnement");
    }
}

/**
 * Fonction pour supprimer une variable d'environnement
 * @param name
 */
void unset_environment_variable(char *name) {
    if (unsetenv(name) != 0) {
        perror("Erreur lors de la suppression de la variable d'environnement");
    }
}

/**
 * Fonction pour afficher les variables d'environnement
 */
void print_environment_variables() {
    extern char **environ;
    for (char **env = environ; *env != NULL; env++) {
        printf("%s\n", *env);
    }
}

/**
 * Fonction pour étendre une variable d'environnement
 * @param token
 * @return
 */
char* expand_variable(const char *token) {
    if (token[0] == '$') {
        const char *value = getenv(token + 1);
        if (value != NULL) {
            return strdup(value);
        } else {
            return strdup("");
        }
        return strdup(token);
    }
}
    /**
    * Fonction pour gérer la flèche Haut
    * @param count
    * @param key
    * @return
    */
    char* read_line_from_file(const char* filename, int target_line) {
        FILE* file = fopen(filename, "r");
        if (file == NULL) {
            perror("Erreur d'ouverture du fichier");
            return NULL;
        }

        char* line = malloc(MAX_LINE_LENGTH);
        if (line == NULL) {
            perror("Erreur d'allocation de mémoire");
            fclose(file);
            return NULL;
        }

        int current_line = 1; // Compteur de ligne
        while (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
            if (current_line == target_line) {
                fclose(file);
                return line; // Retourner la ligne trouvée
            }
            current_line++;
        }

        // Si la ligne n'existe pas
        free(line);
        fclose(file);
        return NULL;
}

void expand_arguments(ParsedCommand *cmd) {
    for (int i = 0; cmd->args[i] != NULL; i++) {
        if (cmd->args[i][0] == '$') {
            char *expanded = getenv(cmd->args[i] + 1); // Ignorer le '$'
            if (expanded) {
                free(cmd->args[i]); // Libérer l'ancien argument
                cmd->args[i] = strdup(expanded); // Remplacer par la valeur
            } else {
                // Remplacer par une chaîne vide si la variable n'existe pas
                free(cmd->args[i]);
                cmd->args[i] = strdup("");
            }
        }
    }
}
