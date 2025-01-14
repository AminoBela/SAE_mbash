#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#define MAXLI 2048

char cmd[MAXLI];
char path[MAXLI];
int pathidx;
void mbash();

void history();
void save_history();
void clear_history();


int main(int argc, char** argv) {

  while (1) {

    printf(">");
    fgets(cmd, MAXLI, stdin);
    strcpy(cmd, cmd);

    mbash(cmd);

  }
  return 0;

}

void mbash() {

  if (strcmp(cmd, "history\n") == 0) {
    save_history(cmd);
    history();
  }

  else if (strcmp(cmd, "history -c\n") == 0) {
    clear_history();
  }

  else {
    save_history(cmd);
    system(cmd);
  }

}


void history() {

  // Ouvrir le fichier historique.txt en mode lecture
  FILE *file = fopen("history.txt", "r");
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

void save_history() {
  // Ouvrir le fichier en mode ajout
  FILE* file = fopen("history.txt", "a");

  // Si l'utilisateur n'as pas juste appuyé sur entrer
  if (strcmp("\n", cmd) != 0 && strcmp("", cmd) != 0) {
    // Écrire la commande dans le fichier
    fprintf(file, "%s", cmd);
  }

  fclose(file);

}

void clear_history() {

  FILE* file = fopen("history.txt", "w");
  if (file != NULL) {
    remove("history.txt");
    history();
  }
}