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

char* historique[20];
int size = 0;


void history() {

  for (int i = 0; i < size; i++) {
    printf(": %s\n", historique[i]);
  }

}


int main(int argc, char** argv) {
  while (1) {
    printf("Commande: ");
    fgets(cmd, MAXLI, stdin);
    if (strcmp(cmd, "history") == 0) {
      history();
    }
    mbash(cmd);
  }
  return 0;
}

void mbash() {

  // Ajout de la commande dans l'historique
  if (size < 20) {

    historique[size] = strdup(cmd);
    size++;

  }

  printf("Execute: %s", cmd);
  system(cmd);

}