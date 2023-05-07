#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sem.h>

//wait-Implementierung
void wait_sem(int semid, int semnum) {
    struct sembuf sops;
    sops.sem_num = semnum;
    sops.sem_op = -1;
    sops.sem_flg = 0;

	//Operation auf Semaphore ausführen
    if (semop(semid, &sops, 1) == -1) {
        perror("Fehler in wait-Implementierung\n");
        exit(1);
    }
}

//signal-Implementierung
void signal_sem(int semid, int semnum) {
    struct sembuf sops;
    sops.sem_num = semnum;
    sops.sem_op = 1;
    sops.sem_flg = 0;

	//Operation auf Semaphore ausführen
    if (semop(semid, &sops, 1) == -1) {
        perror("Fehler in signal-Implementierung\n");
        exit(1);
    }
}

int main() {
	printf("Diese Lösung wurde erstellt von <Vorname> <Nachname>\n");

    //Semaphoren anlegen
	int semid = semget(IPC_PRIVATE, 2, 0777 | IPC_CREAT);
    if (semid == -1) {
        perror("Fehler beim Anlegen der Semaphoren\n");
        exit(1);
    }

	//Zugriff auf Semaphoren
    semctl(semid, 0, SETALL, 0);

	//Kindprozess erzeugen
    int pid = fork();
    if (pid == -1) {
        perror("Fehler beim Erzeugen des Kindprozesses\n");
        exit(1);
    } else if (pid == 0) {
        //Kindprozess
        printf("Kindprozess gestartet\n");
        wait_sem(semid, 0); //auf Signal des Vaterprozesses warten
        printf("Kindprozess hat Signal von Vaterprozess erhalten\n");
        printf("Nachricht eingeben: ");
        fflush(stdout);
        char msg[100];
        fgets(msg, 100, stdin);
        signal_sem(semid, 1); //dem Vaterprozess signalisieren
		printf("Kindprozess hat Signal zu Vaterprozess gesendet\n");
        printf("Kindprozess beenden\n");
        exit(0);
    } else {
        //Vaterprozess
        printf("Vaterprozess gestartet\n");
        printf("Nachricht eingeben: ");
        fflush(stdout);
        char msg[100];
        fgets(msg, 100, stdin);
        signal_sem(semid, 0); //dem Kindprozess signalisieren
        printf("Vaterprozess hat Signal zu Kindprozess gesendet\n");
        wait_sem(semid, 1); //auf Signal des Kindprozesses warten
        printf("Vaterprozess hat Signal von Kindprozess erhalten\n");
        printf("Vaterprozess beenden\n");

		//Semaphorenfeld löschen
        semctl(semid, 0, IPC_RMID, 0);
    }
    return 0;
}
