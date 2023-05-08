//
// Created by #Studis19# on 14.04.23.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <errno.h>

#define INPUT_LENGHT 1000


void setColor(int color) {
    printf("\033[0;%dm", color);
}

void printPrompt() {
    setColor(32);
    char hostname[1024];
    gethostname(hostname, 1024);
    printf("%s@%s", getenv("USER"), hostname);
    setColor(0);
    printf(":");
    setColor(34);
    printf("%s", getenv("PWD"));
    setColor(0);
    printf("$ ");
}

int main(int argc, char *argv[]) {
    char input[INPUT_LENGHT];


    while (1) {
        printPrompt();
        fgets(input, INPUT_LENGHT, stdin);
        input[strlen(input) - 1] = '\0';

        //Bei Eingabe "schluss" Midi-Shell beenden
        if (strcmp(input, "schluss") == 0) {
            printf("Ich zerstöre mich jetzt selbst!\n");
            break;
        }

        //Anzahl der Leerzeichen ermitteln
        int spaces = 0;
        for (int i = 0; i < strlen(input); i++) {
            if (input[i] == ' ') {
                spaces++;
            }
        }

        //Eingabe in Parameter-Liste umwandeln
        // Anzahl der Leerzeichen + 2, da NULL-Terminierung
        char **params = (char **) malloc((spaces + 2) * sizeof(char *));
        params[0] = strtok(input, " ");

        for (int i = 1; i < spaces + 1; ++i) {
            params[i] = strtok(NULL, " ");
        }
        params[spaces + 1] = NULL;
        char *command = params[0];

        int child = fork();

        if(child == -1) {
            fprintf(stderr, "Fehler beim Erzeugen des Kind-Prozesses");
            return 1; //TODO Überprüfen, ob diese IF passt
        } else if (child == 0) {
            int commandLength = (int) strlen(command);
            if (command[0] == '/') {
                execv(command, params);
            //TODO execv kann ./ und ../ von alleine... :D
            } else if ((command[0] == '.' && command[1] == '/')
                        || (command[0] == '.' && command[1] == '.' && command[2] == '/')) {
                char *pwd = getenv("PWD");
                int pwdLength = (int) strlen(pwd);
                char *fullCommand = (char *) malloc((pwdLength + commandLength + 1) * sizeof(char));
                strcpy(fullCommand, pwd);
                strcat(fullCommand, "/");
                strcat(fullCommand, command);
                execv(fullCommand, params);
                free(fullCommand);
            } else {
                //Programme im Standardverzeichnis suchen
                char *paths = getenv("PATH");
                // Path hat folgenden Aufbau: /usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin:/snap/bin
                int pathsLength = (int) strlen(paths);
                char *splitPath;
                char *commandPath = (char *) malloc((pathsLength + commandLength + 1) * sizeof(char));
                //char *pathCopy = (char *) malloc((pathsLength + 1) * sizeof(char));
                //strcpy(pathCopy, paths);
                splitPath = strtok(paths, ":"); //hier war pathCopy
                //strcpy(splitPathCopy, splitPath);
                //strcat(splitPathCopy, "/");
                //strcat(splitPathCopy, command);
                //execv(splitPathCopy, params);
                while (splitPath != NULL) {
                    strcpy(commandPath, splitPath);
                    strcat(commandPath, "/");
                    strcat(commandPath, command);
                    execv(commandPath, params);
                    splitPath = strtok(NULL, ":");
                }
                free(commandPath);
            }
            fprintf(stderr, "Fehler: '%s' nicht gefunden (%s)\n", command, strerror(errno));
            exit(1);
        } else {
            int status;
            pid_t pid = wait(&status);
            printf("Kindprozess mit pid %i beendet mit Status %i\n", (int) pid, (int) status);
        }
        //TODO Parent-Prozess ist hier nicht benötigt?
        
        free(params);

    }

    //TODO return 0;?


}
