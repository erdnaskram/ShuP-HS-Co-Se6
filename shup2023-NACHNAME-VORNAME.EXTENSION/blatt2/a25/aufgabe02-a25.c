//
// Created by #Studis19# on 14.04.23.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>


void setColor(int color) {
    printf("\033[0;%dm", color);
}

void printPrompt() {
    setColor(32);
    printf("%s@HOST-NAME", getenv("USER"), getenv("PWD"));
    setColor(0);
    printf(":");
    setColor(34);
    printf("%s ", getenv("PWD"));
    setColor(0);
    printf("$ ");
}

int main(int argc, char *argv[]) {
    char input[1000];


    while (1) {
        printPrompt();
        fgets(input, 1000, stdin);
        input[strlen(input) - 1] = '\0';

        if (strcmp(input, "schluss") == 0) {
            printf("ich zerstöre mich jetzt selbst!\n");
            break;
        }


        int spaces = 0;
        for (int i = 0; i < strlen(input); i++) {
            if (input[i] == ' ') {
                spaces++;
            }
        }

        char **params = (char **) malloc((spaces + 2) * sizeof(char *));

        params[0] = strtok(input, " ");

        for (int i = 1; i < spaces + 1; ++i) {
            params[i] = strtok(NULL, " ");
            printf("%s\n", params[i]);
        }
        params[spaces + 1] = NULL;
        char *command = params[0];

        printf("Command %s\n", command);

        if (!(command[0] == '/'
            || (command[0] == '.' && command[1] == '/')
            || (command[0] == '.' && command[1] == '.' && command[2] == '/'))) {

              // /bin:/usr/bin:/usr/local/bin

        }

        int child = fork();

        if (child != -1) {
            if (child == 0) {


                execv(command, params);
                printf("Error File not found: %s\n", command);
                exit(1);
            } else {
                int status;
                pid_t pid = wait(&status);
                printf("Kindprozess mit pid %i beendet mit Status %i\n", (int) pid, (int) status);
            }
        }
        free(params);

    }


}