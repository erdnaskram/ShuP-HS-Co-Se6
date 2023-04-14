#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


int main(int argc, char *argv[]) {
    int child = fork();

    //Child
    if (child == 0) {

        if (argc == 2) {
            execl(argv[1], argv[1], "", "", (char *) NULL);
        } else {

            for (int i = 1; i < argc; i++) {
                argv[i - 1] = argv[i];
            }
            argv[argc - 1] = NULL;

            execv(argv[0], argv);
        }

        printf("EXIT #####1#####\n");
        printf("Error File not found: %s\n", argv[0]);
        exit(1);

    }//Parent
    else {
        int status;
        pid_t pid = wait(&status);
        printf("Kindprozess mit pid %i beendet mit Status %i\n", (int) pid, (int) status);
    }
}

