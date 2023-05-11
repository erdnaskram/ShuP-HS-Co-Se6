#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>


int main(int argc, char *argv[]) {
	printf("Diese Lösung wurde erstellt von Christoph Marks\n");

    if (argc == 1) {
        fprintf(stderr, "Fehler: Es muss ein Programm als Aufrufparameter mitgegeben werden\n");
        return 1;
    }

    int child = fork();

    if (child == -1) {
        perror("Fehler beim Erzeugen des Kind-Prozesses");
        return 1;
    }

    if (child == 0) {
        //Kindprozess

        //Argumente für Übergabe an execv vorbereiten
        for (int i = 1; i < argc; i++) {
            argv[i - 1] = argv[i];
        }
        argv[argc - 1] = NULL;

        execv(argv[0], argv);

        fprintf(stderr,"Fehler! Argument: %s; Fehler-Nummer: %i (%s)\n", argv[0], errno, strerror(errno));
        exit(errno);

    } else {
        //Vaterprozess

        int status;
        int pid = wait(&status);
        printf("Kindprozess mit pid %i beendet mit Status %i\n", pid, status);

        return 0;
    }
}
