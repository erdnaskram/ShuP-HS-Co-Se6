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

void sigterm_handler(int);
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


int get_sem(int semId, int semNum) {
    union semun {
        int val;
        struct semid_ds *buf;
        ushort *array;
    } arg;
    int val = semctl(semId, semNum, GETVAL, arg);
    return val > 0 ? val : 0;
}



int main() {
    printf("Parent-Prozess-ID: %d\n", getpid());

    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler);

    //Shared Memory-Bereich erzeugen
    //5 int-Werte (jeweils SharedMemoryID für 1 Child) + 2 int-Werte (next to read + next to write) = 7 int-Werte
    int shm_id = shmget(IPC_PRIVATE, 7 * sizeof(int), 0777 | IPC_CREAT);
    if (shm_id < 0) {
        printf("Fehler beim Erzeugen des Shared Memory-Bereichs für Warteschlange + Counter\n");
        return 1;
    }

    //Initialisierung des Shared Memory-Bereichs
    int *shared_mem = (int *) shmat(shm_id, NULL, 0);
    shared_mem[5] = 0; // Next to Write
    shared_mem[6] = 0; // Next to Read
    shmdt(shared_mem);
    
    
    /**Kommunikation Spooler -> Drucker*/
    int shm_drucker_id = shmget(IPC_PRIVATE, 2 * sizeof(int), 0777 | IPC_CREAT);
    if (shm_drucker_id < 0) {
        perror("Fehler beim Erzeugen des Shared Memory-Bereichs für Drucker\n");
        return 1;
    }


    /**Shared Memory-Bereich für Steuervariable run*/
    int shm_run_id = shmget(IPC_PRIVATE, sizeof(int), 0777 | IPC_CREAT);
    if (shm_run_id < 0) {
        perror("Fehler beim Erzeugen des Shared Memory-Bereichs für Steuervariable run\n");
        return 1;
    }

    //Initialisierung des Shared Memory-Bereichs
    shared_run_mem = (int *) shmat(shm_run_id, NULL, 0);
    *shared_run_mem = 1;
    

    /**Semaphoren Anwendungen <-> Druckerspooler*/
    int semid_ringspeicher = semget(IPC_PRIVATE, 3, 0777 | IPC_CREAT);
    if (semid_ringspeicher == -1) {
        perror("Fehler in semget von Anwendungen und Druckerspooler");
        exit(1);
    }
    
    //Initialisierung der Semaphoren
    //0. Sem = mutex
    semctl(semid_ringspeicher, 0, SETVAL, 1);
    //1. Sem = erzeuger, Warteschlange leer
    semctl(semid_ringspeicher, 1, SETVAL, 5);
    //2. Sem = verbraucher, Warteschlange voll
    semctl(semid_ringspeicher, 2, SETVAL, 0);
    
    
    /**Semaphoren Druckerspooler <-> Drucker*/
    int semid_druckerkommunikation = semget(IPC_PRIVATE, 4, 0777 | IPC_CREAT);
    if (semid_druckerkommunikation == -1) {
        perror("Fehler in semget von Druckerspooler und Druckern");
        exit(1);
    }

    //Initialisierung der Semaphoren
    //0. Sem = drucker1, Warteschlange leer
    semctl(semid_druckerkommunikation, 0, SETVAL, 1);
    //1. Sem = drucker2, Warteschlange leer
    semctl(semid_druckerkommunikation, 1, SETVAL, 1);
    //2. Sem = drucker1, Warteschlange voll
    semctl(semid_druckerkommunikation, 2, SETVAL, 0);
    //3. Sem = drucker2, Warteschlange voll
    semctl(semid_druckerkommunikation, 3, SETVAL, 0);
    

    
    /**
     * Spooler-Prozess
     * */
    int spooler = fork();

    if (spooler == -1) {
        perror("Fehler beim Erzeugen des Spooler-Prozesses");
        return 1;
    } else if (spooler == 0) {
        shared_mem = (int *) shmat(shm_id, NULL, 0);
        int *shared_drucker_mem = (int *) shmat(shm_drucker_id, NULL, 0);
        int drucker_turn = 0;

        while (*shared_run_mem) {
            int *next_to_read = &shared_mem[6];

            //Warten bis etwas in Druckwarteschlange steht
            wait_sem(semid_ringspeicher, 2);

            //Schleife frühzeitig beim Beenden verlassen
            if (!*shared_run_mem){
                //Um das Abbrechen während arbeitsbeginn zu handeln wird die Druckerwarteschlange wieder 1 hochgezählt
                signal_sem(semid_ringspeicher, 2);
                continue;
            }

            int shared_mem_child_id = shared_mem[*next_to_read];

            //auf druckenden Drucker warten
            wait_sem(semid_druckerkommunikation, drucker_turn);

            //Schleife frühzeitig beim Beenden verlassen
            if (!*shared_run_mem)
                continue;

            shared_drucker_mem[drucker_turn] = shared_mem_child_id;
            //Druckplatz belegen
            signal_sem(semid_druckerkommunikation, drucker_turn + 2);

            //einen Platz in Druckerwarteschlange freigeben
            signal_sem(semid_ringspeicher, 1);

            //Druckaufträge abwechselnd an Drucker verteilen
            drucker_turn++;
            if (drucker_turn == 2) {
                drucker_turn = 0;
            }

            //im Ringspeicher eins weiter zählen
            (*next_to_read)++;
            if (*next_to_read == 5) {
                *next_to_read = 0;
            }
        }

        printf("Beachten Sie, dass Druckaufträge möglicherweise nicht ausgeführt werden konnten, da das Programm beendet wurde!\n");

        //Shared Memory ausblenden
        shmdt(shared_run_mem);
        shmdt(shared_mem);
        shmdt(shared_drucker_mem);

        return 0;
    }

    
    
    /**
     * Drucker1-Prozess
     * */
    int drucker1 = fork();

    if (drucker1 == -1) {
        perror("Fehler beim Erzeugen des Drucker1-Prozesses");
        return 1;
    } else if (drucker1 == 0) {
        int drucker_nr = 0;
        int *shared_drucker_mem = (int *) shmat(shm_drucker_id, NULL, 0);

        while (*shared_run_mem) {
            //auf Druckauftrag warten
            wait_sem(semid_druckerkommunikation, drucker_nr + 2);

            //Schleife frühzeitig beim Beenden verlassen
            if (!*shared_run_mem)
                continue;

            int to_print_id = shared_drucker_mem[drucker_nr];
            int *shared_child_mem = (int *) shmat(to_print_id, NULL, 0);

            //Druckvorgang
            for (int i = 1; i <= shared_child_mem[0]; i++) {
                printf("\033[0;32mDrucker %i druckt Druckauftrag mit ID %i: Seite %i mit Inhalt %i\033[0;0m\n",
                       drucker_nr + 1, to_print_id, i, shared_child_mem[i]);
                sleep(1);
                printf("\033[0;32mDrucker %i hat Druckauftrag mit ID %i gedruckt: Seite %i\033[0;0m\n",
                       drucker_nr + 1, to_print_id, i);
            }

            //Shared Memory ausblenden
            shmdt(shared_child_mem);
            //Shared Memory freigeben
            shmctl(to_print_id, IPC_RMID, NULL);
            //Druckplatz freigeben
            signal_sem(semid_druckerkommunikation, drucker_nr);
        }

        //Shared Memory ausblenden
        shmdt(shared_run_mem);
        shmdt(shared_drucker_mem);

        return 0;
    }


    
    /**
     * Drucker2-Prozess
     * */
    int drucker2 = fork();

    if (drucker2 == -1) {
        perror("Fehler beim Erzeugen des Drucker2-Prozesses");
        return 1;
    } else if (drucker2 == 0){
        int drucker_nr = 1;
        int *shared_drucker_mem = (int *) shmat(shm_drucker_id, NULL, 0);

        while (*shared_run_mem) {
            //auf Druckauftrag warten
            wait_sem(semid_druckerkommunikation, drucker_nr + 2);

            //Schleife frühzeitig beim Beenden verlassen
            if (!*shared_run_mem)
                continue;

            int to_print_id = shared_drucker_mem[drucker_nr];
            int *shared_child_mem = (int *) shmat(to_print_id, NULL, 0);

            //Druckvorgang
            for (int i = 1; i <= shared_child_mem[0]; i++) {
                printf("\033[0;93mDrucker %i druckt Druckauftrag mit ID %i: Seite %i mit Inhalt %i\033[0;0m\n",
                       drucker_nr + 1, to_print_id, i, shared_child_mem[i]);
                sleep(1);
                printf("\033[0;93mDrucker %i hat Druckauftrag mit ID %i gedruckt: Seite %i\033[0;0m\n",
                       drucker_nr + 1, to_print_id, i);
            }

            //Shared Memory ausblenden
            shmdt(shared_child_mem);
            //Shared Memory freigeben
            shmctl(to_print_id, IPC_RMID, NULL);
            //Druckplatz freigeben
            signal_sem(semid_druckerkommunikation, drucker_nr);
        }

        //Shared Memory ausblenden
        shmdt(shared_run_mem);
        shmdt(shared_drucker_mem);
        
        return 0;
    }


    
    /**
     * Kindprozesse Init
     * */
    //Variablen für Kindprozesse
    int pid;
    srand(time(NULL));
    int children_counter = 0;

    /**
     * Kindprozesse
     * */
    while (*shared_run_mem) {
        //Anzahl der Kinder mitzählen, um Shared Memory freizugeben
        children_counter++;
        sleep(rand() % 5);
        pid = fork();
        
        if (pid == -1) {
            perror("Fehler beim Erzeugen des Kindprozesses\n");
            return 1;
        } else if (pid == 0) { // wennn pid = 0 ist, ist es ein Kindprozess
            shared_mem = (int *) shmat(shm_id, NULL, 0);

            int pages = rand() % 5 + 2;

            //einen Platz in Druckerwarteschlange belegen
            wait_sem(semid_ringspeicher, 1);
            
            //Schleife frühzeitig beim Beenden verlassen
            if (!*shared_run_mem)
                exit(0);

            //auf Schreibzugriff warten
            wait_sem(semid_ringspeicher, 0);

            //Schleife frühzeitig beim Beenden verlassen
            if (!*shared_run_mem)
                exit(0);

            int shm_child_id = shmget(IPC_PRIVATE, (pages + 1) * sizeof(int), 0777 | IPC_CREAT);
            int *shared_child_mem = (int *) shmat(shm_child_id, NULL, 0);

            // Schreiben der Anzahl der Seiten in den ersten Speicherplatz
            shared_child_mem[0] = pages;
            for (int i = 1; i <= pages; i++) {
                // Schreiben des Druckinhalts in den Speicherplatz
                shared_child_mem[i] = rand();
                printf("Anwendung erzeugt Druckauftrag mit ID %i Seite %i mit Inhalt %i\n", shm_child_id, i,
                       shared_child_mem[i]);
            }

            int *next_to_write = &shared_mem[5];
            shared_mem[*next_to_write] = shm_child_id;

            //im Ringspeicher eins weiter zählen, wenn 5 dann wieder auf 0
            (*next_to_write)++;
            if (*next_to_write == 5) {
                *next_to_write = 0;
            }

            //Schreibzugriff abgeben
            signal_sem(semid_ringspeicher, 0);
            //Spooler über Druckauftrag informieren
            signal_sem(semid_ringspeicher, 2);

            //Shared Memory ausblenden
            shmdt(shared_mem);
            shmdt(shared_child_mem);
            shmdt(shared_run_mem);

            return 0; //sonst gehen Kinder auch wieder in Schleife
        }

    }

    
    
    /**
     * Beenden des Programms
     * */
    
    //Entfernen der SharedMemory's der Kindprozesse, welche in der Warteschlange stehen
    //& noch keinem Drucker zugewiesen wurden
    shared_mem = (int *) shmat(shm_id, NULL, 0);
    //So lange durchlaufen bis LesePos gleich NextSchreibePos,
    //falls beide zu Beginn gleich sind, prüfen ob Warteschlange komplett voll
    while (shared_mem[6] != shared_mem[5] || get_sem(semid_ringspeicher, 1) == 0){
        int toEndID = shared_mem[shared_mem[6]];
        shmctl(toEndID, IPC_RMID, NULL);
        shared_mem[6]++;
        if (shared_mem[6] == 5){
            shared_mem[6] = 0;
        }
        //signal der Sem um Endlosschleife zu verhindern
        signal_sem(semid_ringspeicher, 1);
    }

    //bei feststeckenden Prozessen in waits
    signal_sem(semid_druckerkommunikation, 2); //Drucker1 aus wait holen
    signal_sem(semid_druckerkommunikation, 3); //Drucker2 aus wait holen
    signal_sem(semid_druckerkommunikation, 1); //Spooler aus wait für Drucker2 holen
    signal_sem(semid_druckerkommunikation, 0); //Spooler aus wait für Drucker1 holen
    signal_sem(semid_ringspeicher, 2); //Spooler aus wait holen

    for (int i = 0; i < children_counter; i++) {
        //Anwendungen aus wait holen
        signal_sem(semid_ringspeicher, 1);
        signal_sem(semid_ringspeicher, 0);
    }

    //Warten auf Beenden der Kind-Prozesse
    while (wait(NULL) != -1);

    //Semaphoren löschen
    semctl(semid_ringspeicher, 0, IPC_RMID, 0);
    semctl(semid_druckerkommunikation, 0, IPC_RMID, 0);

    // Shared Memory-Bereich detachen
    shmdt(shared_run_mem);

    // Shared Memory-Bereich löschen
    shmctl(shm_id, IPC_RMID, NULL);
    shmctl(shm_drucker_id, IPC_RMID, NULL);
    shmctl(shm_run_id, IPC_RMID, NULL);

    return 0;
}



void sigterm_handler(int signb) {
    *shared_run_mem = 0;
}