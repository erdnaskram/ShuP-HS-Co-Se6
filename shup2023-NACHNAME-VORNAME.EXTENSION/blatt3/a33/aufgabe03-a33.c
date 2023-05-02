/*
 * aufgabe03-a33.c
 * Copyright (C) 2023 Zoe Schulz <zoe.schulz@mail.de>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/sem.h>

void sigtermhandler(int);

int *shared_run_mem;

void wait_sem(int semid, int semnum) {
    struct sembuf sops;
    sops.sem_num = semnum;
    sops.sem_op = -1;
    sops.sem_flg = 0;
    if (semop(semid, &sops, 1) == -1) {
        perror("semop failed");
        exit(1);
    }
}

void signal_sem(int semid, int semnum) {
    struct sembuf sops;
    sops.sem_num = semnum;
    sops.sem_op = 1;
    sops.sem_flg = 0;
    if (semop(semid, &sops, 1) == -1) {
        perror("semop failed");
        exit(1);
    }
}

int main() {
	printf("Parent-Prozess-ID: %d\n", getpid());

    signal(SIGTERM, sigtermhandler);
    signal(SIGINT, sigtermhandler);

    // 	Shared Memory-Bereich erzeugen
    // 	5 int-Werte (jeweils SharedMemoryID für 1 Child) + 2 int-Werte (next to read + next to write) = 7 int-Werte
    int shm_id = shmget(IPC_PRIVATE, 7 * sizeof(int), 0777 | IPC_CREAT);
    if (shm_id < 0) {
        printf("Fehler beim Erzeugen des Shared Memory-Bereichs für Warteschlange + Counter\n");
        return 1;
    }

	//Kommunikation Spooler -> Drucker
    int shm_drucker_id = shmget(IPC_PRIVATE, 2 * sizeof(int), 0777 | IPC_CREAT);
    if (shm_drucker_id < 0) {
        printf("Fehler beim Erzeugen des Shared Memory-Bereichs für Drucker\n");
        return 1;
    }

    int *shared_mem = (int *) shmat(shm_id, NULL, 0);
    shared_mem[5] = 0; // Next to Write
    shared_mem[6] = 0; // Next to Read
    shmdt(shared_mem);


    //Semaphoren Anwendungen <-> Druckerspooler
    int semid = semget(IPC_PRIVATE, 3, 0777 | IPC_CREAT);
    if (semid == -1) {
        perror("Fehler in semget von Anwendungen und Druckerspooler"); //TODO: Ausgabe von errno
        exit(1);
    }

	//0. Sem = mutex
    semctl(semid, 0, SETVAL, 1);
	//1. Sem = erzeuger, Warteschlange leer
	semctl(semid, 1, SETVAL, 5);
	//2. Sem = verbraucher, Warteschlange voll
	semctl(semid, 2, SETVAL, 0);


    //Semaphoren Druckerspooler <-> Drucker
    int semid2 = semget(IPC_PRIVATE, 4, 0777 | IPC_CREAT);
    if (semid2 == -1) {
        perror("Fehler in semget von Druckerspooler und Druckern"); //TODO: Ausgabe von errno
        exit(1);
    }

	//0. Sem = drucker1, Warteschlange leer
    semctl(semid2, 0, SETVAL, 1);
	//1. Sem = drucker2, Warteschlange leer
	semctl(semid2, 1, SETVAL, 1);
	//2. Sem = drucker1, Warteschlange voll
    semctl(semid2, 2, SETVAL, 0);
	//3. Sem = drucker2, Warteschlange voll
	semctl(semid2, 3, SETVAL, 0);

    int shm_run_id = shmget(IPC_PRIVATE, sizeof(int), 0777 | IPC_CREAT);
    if (shm_run_id < 0) {
        printf("Fehler beim Erzeugen des Shared Memory-Bereichs für Steuervariable run\n");
        return 1;
    }

	shared_run_mem = (int *) shmat(shm_run_id, NULL, 0);
	*shared_run_mem = 1;

    int spooler = fork();

    if (spooler == -1) {
        printf("Fehler beim Erzeugen des Spooler-Prozesses");
        return 1;
    } else if (spooler == 0) {
        printf("Spoooooooler\n");

        int *shared_mem = (int *) shmat(shm_id, NULL, 0);
        int *shared_drucker_mem = (int *) shmat(shm_drucker_id, NULL, 0);
		int druckerTurn = 0;

        while (*shared_run_mem) {// Hinweis, dass noch Geschriebenes nicht mehr gedruckt wird, reicht.
            int *nextToRead = &shared_mem[6];

			wait_sem(semid, 2); //Warten bis etwas in Druckwarteschlange steht

			//frühzeitig Schleife bei Beenden verlassen
			if(!*shared_run_mem)
				continue;

			int sharedMemChildID = shared_mem[*nextToRead];
			signal_sem(semid, 1); //einen Platz in Druckerwarteschlange frei geben

			wait_sem(semid2, druckerTurn); //auf druckenden Drucker warten

			//frühzeitig Schleife bei Beenden verlassen
			if(!*shared_run_mem)
				continue;

			shared_drucker_mem[druckerTurn] = sharedMemChildID;
			signal_sem(semid2, druckerTurn+2); //Druckplatz belegen

			//Druckaufträge abwechselnd an Drucker verteilen
			druckerTurn++;
			if(druckerTurn == 2){
				druckerTurn = 0;
			}

            //im Ringspeicher eins weiter zählen
            (*nextToRead)++;
            if (*nextToRead == 5) {
                *nextToRead = 0;
            }

        }
	
	printf("Beachten Sie, dass Druckaufträge möglicherweise nicht ausgeführt werden konnten, da das Programm beendet wurde!\n");

	//Shared Memory detached
        shmdt(shared_run_mem);
        //detach shared memory
        shmdt(shared_mem);
	shmdt(shared_drucker_mem);

        return 0;
    }

    // TODO: Jeder Drucker druckt nur einen Auftrag...

    int drucker1 = fork();

    if (drucker1 == -1) {
        printf("Fehler beim Erzeugen des Drucker1-Prozesses");
        return 1;
    } else if (drucker1 == 0) {
        printf("Drucker1\n");

		int druckerNR = 0;
        int *shared_drucker_mem = (int *) shmat(shm_drucker_id, NULL, 0);

		while(*shared_run_mem){
			wait_sem(semid2, druckerNR+2); //auf Druckauftrag warten

			//frühzeitig Schleife bei Beenden verlassen
			if(!*shared_run_mem)
				continue;

			int toPrintID = shared_drucker_mem[druckerNR];
       		int *shared_child_mem = (int *) shmat(toPrintID, NULL, 0);

			for(int i = 1; i <= shared_child_mem[0]; i++){
				printf("\033[0;32mDrucker %i druckt Druckauftrag mit ID %i: Seite %i mit Inhalt %i\033[0;0m\n", druckerNR+1, toPrintID, i, shared_child_mem[i]);
				sleep(1);
				printf("\033[0;32mDrucker %i hat Druckauftrag mit ID %i gedruckt: Seite %i\033[0;0m\n", druckerNR+1, toPrintID, i);
			}

			//Shared Memory ausblenden
			shmdt(shared_child_mem);
            shmdt(shared_run_mem);

			//Shared Memory freigeben
			shmctl(toPrintID, IPC_RMID, NULL);

			signal_sem(semid2, druckerNR); //Druckplatz freigeben
		}

		//Shared Memory ausblenden
		shmdt(shared_drucker_mem);


        return 0;
    }


    int drucker2 = fork();

    if (drucker2 == -1) {
        printf("Fehler beim Erzeugen des Drucker2-Prozesses");
        return 1;
    } else if (drucker2 == 0) {

        printf("Drucker2\n");

		int druckerNR = 1;
        int *shared_drucker_mem = (int *) shmat(shm_drucker_id, NULL, 0);

		printf("SRM: %i\n", *shared_run_mem);

		while(*shared_run_mem){
			printf("D2\n");
			wait_sem(semid2, druckerNR+2); //auf Druckauftrag warten
			printf("D2 1\n");
			//frühzeitig Schleife bei Beenden verlassen
			if(!*shared_run_mem)
				continue;

			int toPrintID = shared_drucker_mem[druckerNR];
       		int *shared_child_mem = (int *) shmat(toPrintID, NULL, 0);

			for(int i = 1; i <= shared_child_mem[0]; i++){
				printf("\033[0;93mDrucker %i druckt Druckauftrag mit ID %i: Seite %i mit Inhalt %i\033[0;0m\n", druckerNR+1, toPrintID, i, shared_child_mem[i]);
				sleep(1);
				printf("\033[0;93mDrucker %i hat Druckauftrag mit ID %i gedruckt: Seite %i\033[0;0m\n", druckerNR+1, toPrintID, i);
			}

			//Shared Memory ausblenden
			shmdt(shared_child_mem);
            shmdt(shared_run_mem);

			//Shared Memory freigeben
			shmctl(toPrintID, IPC_RMID, NULL);

			signal_sem(semid2, druckerNR); //Druckplatz freigeben
		}

		//Shared Memory ausblenden
		shmdt(shared_drucker_mem);


        return 0;
    }


    int pid;
    srand(time(NULL));
	int childrenCounter = 0;

    while (*shared_run_mem) {
		childrenCounter++;
        sleep(rand() % 5);
        pid = fork();
        if (pid == -1) {
            printf("Fehler beim Erzeugen des Kindprozesses\n");
            return 1;
        } else if (pid == 0) { // wennn pid = 0 ist, ist es ein Kindprozess
            int *shared_mem = (int *) shmat(shm_id, NULL, 0);

            int pages = rand() % 5 + 2;

            int shm_child_id = shmget(IPC_PRIVATE, (pages + 1) * sizeof(int), 0777 | IPC_CREAT);
            int *shared_child_mem = (int *) shmat(shm_child_id, NULL, 0);

            shared_child_mem[0] = pages; // Schreiben der Anzahl der Seiten in den ersten Speicherplatz
            for (int i = 1; i <= pages; i++) {
                shared_child_mem[i] = rand();// Schreiben des Druckinhalts in den Speicherplatz
		printf("Anwendung erzeugt Druckauftrag mit ID %i Seite %i mit Inhalt %i\n", shm_child_id, i, shared_child_mem[i]);
            }

			wait_sem(semid, 1); //einen Platz in Druckerwarteschlange belegen

			//frühzeitig Schleife bei Beenden verlassen
			if(!*shared_run_mem){
				exit(0);
			}
			wait_sem(semid, 0); //auf Schreibzugriff warten

			//frühzeitig Schleife bei Beenden verlassen
			if(!*shared_run_mem)
				exit(0);
            int *nextToWrite = &shared_mem[5];
            shared_mem[*nextToWrite] = shm_child_id;

            //im Ringspeicher eins weiter zählen
            (*nextToWrite)++;
            if (*nextToWrite == 5) {
                *nextToWrite = 0;
            }

			signal_sem(semid, 0); //Schreibzugriff abgeben
			signal_sem(semid, 2); //Spooler über Druckauftrag informieren

            //detach shared memory
            shmdt(shared_mem);
            shmdt(shared_child_mem);
            shmdt(shared_run_mem);

            return 0; //sonst gehen Kinder auch wieder in Schleife
        }

    }

    	// TODO: Von Child-Prozessen erzeugte SHM-Bereiche beenden, die aufgrund der abgeschossenen Drucker nicht mehr automatisch beendet werden

	//bei feststeckenden Prozessen in waits
	signal_sem(semid2, 2); //Drucker1 aus wait holen
	signal_sem(semid2, 3); //Drucker2 aus wait holen
	signal_sem(semid2, 1); //Spooler aus wait für Drucker2 holen
	signal_sem(semid2, 0); //Spooler aus wait für Drucker1 holen
	signal_sem(semid, 2); //Spooler aus wait holen

	for(int i = 0; i<childrenCounter;i++){ //Anwendungen aus wait holen
		signal_sem(semid, 1);
		signal_sem(semid, 0);
	}

	//Warten auf Beenden der Kind-Prozesse
        while (wait(NULL)!=-1) ;

	//Semaphoren löschen
	semctl(semid, 0, IPC_RMID, 0);
	semctl(semid2, 0, IPC_RMID, 0);

	// Shared Memory-Bereich detachen
	shmdt(shared_run_mem);

    // Shared Memory-Bereich löschen
    shmctl(shm_id, IPC_RMID, NULL);
	shmctl(shm_drucker_id, IPC_RMID, NULL);
	shmctl(shm_run_id, IPC_RMID, NULL);

    return 0;


}

void sigtermhandler(int signb) {
    *shared_run_mem = 0;
}
