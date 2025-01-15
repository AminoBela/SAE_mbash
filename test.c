#include <stdio.h>
#include <unistd.h>
#include <limits.h>

int main() {

    char cwd[PATH_MAX]; // Répertoire courant

    printf("\033[31m%s >\033[0m", getcwd(cwd, sizeof(cwd)));

    return 0;
}