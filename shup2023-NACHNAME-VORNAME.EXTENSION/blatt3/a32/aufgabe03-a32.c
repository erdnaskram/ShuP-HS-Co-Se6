#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    printf("Diese Lösung wurde erstellt von <Vorname> <Nachname>\n");

	//Prüfung auf zwei übergebene Argumente
    if (argc != 3) {
        printf("korrekter Aufruf: %s <Anzahl Schleifendurchläufe> <Seed>\n", argv[0]);
        return 1;
    }

    //Anzahl Schleifendurchläufe und Seed aus den Argumenten auslesen
    int anzahlDurchlaeufe = atoi(argv[1]);
    int seed = atoi(argv[2]);

    //Shared Memory-Bereich erzeugen
    int shm_id = shmget(IPC_PRIVATE, sizeof(int), 0777 | IPC_CREAT);
    if (shm_id < 0) {
        perror("Fehler beim Erzeugen des Shared Memory-Bereichs\n");
        return 1;
    }
    printf("Shared Memory ID: %d\n", shm_id);

    //Zufallsgenerator mit Seed initialisieren
    srand(seed);

    //Kindprozess erzeugen
    int pid = fork();
    if (pid == -1) {
        perror("Fehler beim Erzeugen des Kindprozesses\n");
        return 1;
    } else if (pid == 0) {
        //Kindprozess

		//Shared Memory einblenden
        int *shared_mem = (int*) shmat(shm_id, NULL, 0);

		//Zufallszahlen aus Shared Memory lesen
        for (int i = 0; i < anzahlDurchlaeufe; i++) {
            printf("Kindprozess liest: %d\n", *shared_mem);
        }

		//Shared Memory ausblenden
        shmdt(shared_mem);
    } else {
        //Vaterprozess

		//Shared Memory einblenden
        int *shared_mem = shmat(shm_id, NULL, 0);

		//Zufallszahlen in Shared Memory schreiben
        for (int i = 0; i < anzahlDurchlaeufe; i++) {
            *shared_mem = rand();
            printf("Vaterprozess: %d\n", *shared_mem);
        }

		//Shared Memory ausblenden
        shmdt(shared_mem);

		//auf Kindprozess warten
		int status;
		pid_t pid = wait(&status);
		printf("Kindprozess mit pid %i beendet mit Status %i\n", (int) pid, (int) status);

        //Shared Memory-Bereich löschen
        shmctl(shm_id, IPC_RMID, NULL);
    }

    return 0;
}
