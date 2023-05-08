#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/wait.h>

#define SEMAPHORE_MUTEX1 0
#define SEMAPHORE_MUTEX2 1

//wait-Implementierung
void wait_sem(int semid, int semnum) {
    struct sembuf sops;
    sops.sem_num = semnum;
    sops.sem_op = -1;
    sops.sem_flg = 0;

	//Operation auf Semaphore ausführen
    if (semop(semid, &sops, 1) == -1) {
        perror("Fehler bei wait-Ausführung\n");
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
        perror("Fehler bei signal-Ausführung\n");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
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
        wait_sem(semid, SEMAPHORE_MUTEX1); //auf Signal des Vaterprozesses warten
        printf("Kindprozess hat Signal von Vaterprozess erhalten\n");
        printf("Nachricht eingeben: \n");
        char msg[100];
        fgets(msg, 100, stdin);
        signal_sem(semid, SEMAPHORE_MUTEX2); //dem Vaterprozess signalisieren
		printf("Kindprozess hat Signal zu Vaterprozess gesendet\n");
        printf("Kindprozess beenden\n");
        exit(0);
    } else {
        //Vaterprozess
        printf("Vaterprozess gestartet\n");
        printf("Nachricht eingeben: \n");
        char msg[100];
        fgets(msg, 100, stdin);
        signal_sem(semid, SEMAPHORE_MUTEX1); //dem Kindprozess signalisieren
        printf("Vaterprozess hat Signal zu Kindprozess gesendet\n");
        wait_sem(semid, SEMAPHORE_MUTEX2); //auf Signal des Kindprozesses warten
        printf("Vaterprozess hat Signal von Kindprozess erhalten\n");
        printf("Vaterprozess beenden\n");

		//auf Kindprozess warten
		int status;
		pid = wait(&status);
		printf("Kindprozess mit pid %i beendet mit Status %i\n", pid, status);

		//Semaphorenfeld löschen
        semctl(semid, 0, IPC_RMID, 0);
    }
    return 0;
}
