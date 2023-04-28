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

int run = 1;

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
        printf("Fehler beim Erzeugen des Shared Memory-Bereichs\n");
        return 1;
    }

    int *shared_mem = (int *) shmat(shm_id, NULL, 0);
    shared_mem[5] = 0; // Next to Write
    shared_mem[6] = 0; // Next to Read
    shmdt(shared_mem);


    //SEMAPHOREN
    int semid = semget(IPC_PRIVATE, 3, 0777 | IPC_CREAT);
    if (semid == -1) {
        perror("semget failed"); //TODO: Ausgabe von errno
        exit(1);
    }

	//0. Sem = mutex
    semctl(semid, 0, SETVAL, 1);
	//1. Sem = erzeuger
	semctl(semid, 1, SETVAL, 5);
	//2. Sem = verbraucher
	semctl(semid, 2, SETVAL, 0);

    int spooler = fork();

    if (spooler == -1) {
        printf("Fehler beim Erzeugen des Spooler-Prozesses");
        return 1;
    } else if (spooler == 0) {
        printf("Spoooooooler\n");

        int *shared_mem = (int *) shmat(shm_id, NULL, 0);

        while (run) { // Laut Wißmann kein "sauberes" beenden notwendig,
		      // Hinweis, dass noch Geschriebenes nicht mehr gedruckt wird, reicht.
            int *nextToRead = &shared_mem[6];

			wait_sem(semid, 2); //Warten bis etwas in Druckwarteschlange steht
            int sharedMemChildID = shared_mem[*nextToRead];
			signal_sem(semid, 1); //einen Platz in Druckerwarteschlange frei geben


			printf("Spooler: Shared Memory of child ID: %d\n", sharedMemChildID);

            //TODO: an Drucker senden

            //im Ringspeicher eins weiter zählen
            (*nextToRead)++;
            if (*nextToRead == 5) {
                *nextToRead = 0;
            }
        }

	//TODO: Hinweis, dass Geschriebenes nicht mehr gedruckt wird!
	//TODO: Dafür sorgen, dass Children sich beenden, obwohl sie ggf. in Semaphoren-Waits hängen.

        //detach shared memory
        shmdt(shared_mem);

        return 0;
    }

    int drucker1 = fork();

    if (drucker1 == -1) {
        printf("Fehler beim Erzeugen des Drucker1-Prozesses");
        return 1;
    } else if (drucker1 == 0) {
        printf("Drucker1\n");
        return 0;
    }


    int drucker2 = fork();

    if (drucker2 == -1) {
        printf("Fehler beim Erzeugen des Drucker2-Prozesses");
        return 1;
    } else if (drucker2 == 0) {
        printf("Drucker2\n");
        return 0;
    }


    int pid;
    srand(time(NULL));


    while (run) {
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
            }

			wait_sem(semid, 1); //einen Platz in Druckerwarteschlange belegen
			wait_sem(semid, 0); //auf Schreibzugriff warten

            int *nextToWrite = &shared_mem[5];
            shared_mem[*nextToWrite] = shm_child_id;

            //im Ringspeicher eins weiter zählen
            (*nextToWrite)++;
            if (*nextToWrite == 5) {
                *nextToWrite = 0;
            }

			printf("Child: Shared Memory of Child ID: %d\n", shm_child_id);

			signal_sem(semid, 0); //Schreibzugriff abgeben
			signal_sem(semid, 2); //Spooler über Druckauftrag informieren

            //detach shared memory
            shmdt(shared_mem);
            shmdt(shared_child_mem);

            return 0; //sonst gehen Kinder auch wieder in Schleife
        }

    }


    int status;
    while (1) {
        wait(&status);
        if (&status == NULL)
            break;
    }

    // Shared Memory-Bereich freigen
    shmctl(shm_id, IPC_RMID, NULL);

    return 0;


}

void sigtermhandler(int signb) {
    run = 0;
}
