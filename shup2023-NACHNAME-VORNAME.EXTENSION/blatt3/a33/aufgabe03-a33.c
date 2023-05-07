#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/sem.h>

#define SEMAPHORE_MUTEX 0
#define SEMAPHORE_EMPTY 1
#define SEMAPHORE_FULL 2
#define NEXT_TO_WRITE 5
#define NEXT_TO_READ 6

void sigterm_handler(int);
int *shared_run_mem;

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
        perror("Fehler in signal-Implementierung");
        exit(1);
    }
}

//Implementierung für Zugriff auf Semaphorenwert
int get_sem(int semId, int semNum) {
    union semun {
        int val;
        struct semid_ds *buf;
        ushort *array;
    } arg;
    int val = semctl(semId, semNum, GETVAL, arg);
    return val > 0 ? val : 0;
}

int main(int argc, char* argv[]) {
	printf("Diese Lösung wurde erstellt von <Vorname> <Nachname>\n");

    printf("Parent-Prozess-ID: %d\n", getpid());

	//Signalhandler registrieren
    signal(SIGTERM, sigterm_handler); //15
    signal(SIGINT, sigterm_handler); //2

    //Shared Memory-Bereich erzeugen
    //5 int-Werte (jeweils SharedMemoryID für 1 Child) + 2 int-Werte (next to read + next to write) = 7 int-Werte
    int shm_ringspeicher_id = shmget(IPC_PRIVATE, 7 * sizeof(int), 0777 | IPC_CREAT);
    if (shm_ringspeicher_id < 0) {
        perror("Fehler beim Erzeugen des Shared Memory-Bereichs für Warteschlange + Counter\n");
        return 1;
    }

    //Initialisierung des Shared Memory-Bereichs
    int *shm_ringspeicher = (int *) shmat(shm_ringspeicher_id, NULL, 0);
    shm_ringspeicher[NEXT_TO_WRITE] = 0; // Next to Write
    shm_ringspeicher[NEXT_TO_READ] = 0; // Next to Read
    shmdt(shm_ringspeicher);

    //Shared Memory-bereich für Kommunikation Spooler -> Drucker erzeugen
    int shm_drucker_id = shmget(IPC_PRIVATE, 2 * sizeof(int), 0777 | IPC_CREAT);
    if (shm_drucker_id < 0) {
        perror("Fehler beim Erzeugen des Shared Memory-Bereichs für Drucker\n");
        return 1;
    }

    //Shared Memory-Bereich für Steuervariable run erzeugen
    int shm_run_id = shmget(IPC_PRIVATE, sizeof(int), 0777 | IPC_CREAT);
    if (shm_run_id < 0) {
        perror("Fehler beim Erzeugen des Shared Memory-Bereichs für Steuervariable run\n");
        return 1;
    }

    //Initialisierung des Shared Memory-Bereichs
    shared_run_mem = (int *) shmat(shm_run_id, NULL, 0);
    *shared_run_mem = 1;

    //Semaphoren Anwendungen <-> Druckerspooler erzeugen
    int semid_ringspeicher = semget(IPC_PRIVATE, 3, 0777 | IPC_CREAT);
    if (semid_ringspeicher == -1) {
        perror("Fehler in semget von Anwendungen und Druckerspooler");
        exit(1);
    }

    //Initialisierung der Semaphoren
    //0. Sem = mutex
    semctl(semid_ringspeicher, SEMAPHORE_MUTEX, SETVAL, 1);
    //1. Sem = erzeuger, Warteschlange leer
    semctl(semid_ringspeicher, SEMAPHORE_EMPTY, SETVAL, 5);
    //2. Sem = verbraucher, Warteschlange voll
    semctl(semid_ringspeicher, SEMAPHORE_FULL, SETVAL, 0);

    //Semaphoren Druckerspooler <-> Drucker erzeugen
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
		//Shared Memory-Bereiche einblenden
        shm_ringspeicher = (int *) shmat(shm_ringspeicher_id, NULL, 0);
        int *shared_drucker_mem = (int *) shmat(shm_drucker_id, NULL, 0);

        int drucker_turn = 0;

        while (*shared_run_mem) {
            int *next_to_read = &shm_ringspeicher[NEXT_TO_READ];

            //Warten bis etwas in Druckwarteschlange steht
            wait_sem(semid_ringspeicher, SEMAPHORE_FULL);

            //Schleife frühzeitig beim Beenden verlassen
            if (!*shared_run_mem)
                continue;

            int shared_mem_child_id = shm_ringspeicher[*next_to_read];

            //auf druckenden Drucker warten
            wait_sem(semid_druckerkommunikation, drucker_turn);

            //Schleife frühzeitig beim Beenden verlassen
            if (!*shared_run_mem)
                continue;

			//Drucker id für zu druckende Inhalte geben
            shared_drucker_mem[drucker_turn] = shared_mem_child_id;

			//Druckplatz belegen
            signal_sem(semid_druckerkommunikation, drucker_turn + 2);

            //einen Platz in Druckerwarteschlange freigeben
            signal_sem(semid_ringspeicher, SEMAPHORE_EMPTY);

            //Druckaufträge abwechselnd an die beiden Drucker verteilen
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

        //Shared Memory-Bereiche ausblenden
        shmdt(shared_run_mem);
        shmdt(shm_ringspeicher);
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
        //Anzahl der Kinder mitzählen, um Prozesse aus wait zu holen
        children_counter++;
        sleep(rand() % 5);
        pid = fork();

        if (pid == -1) {
            perror("Fehler beim Erzeugen eines Anwendungs-Prozesses\n");
            return 1;
        } else if (pid == 0) {
			//Kindprozess

			//Shared Memory-Bereich einblenden
            shm_ringspeicher = (int *) shmat(shm_ringspeicher_id, NULL, 0);

			//Anzahl zu druckender Seiten
            int pages = rand() % 5 + 2;

            //einen Platz in Druckerwarteschlange belegen
            wait_sem(semid_ringspeicher, SEMAPHORE_EMPTY);

            //Schleife frühzeitig beim Beenden verlassen
            if (!*shared_run_mem)
                exit(0);

            //auf Schreibzugriff warten
            wait_sem(semid_ringspeicher, SEMAPHORE_MUTEX);

            //Schleife frühzeitig beim Beenden verlassen
            if (!*shared_run_mem)
                exit(0);

			//Shared Memory-Bereich erzeugen und einblenden
            int shm_child_id = shmget(IPC_PRIVATE, (pages + 1) * sizeof(int), 0777 | IPC_CREAT);
            int *shared_child_mem = (int *) shmat(shm_child_id, NULL, 0);

            //Schreiben der Anzahl der Seiten in den ersten Speicherplatz
            shared_child_mem[0] = pages;

			//Schreiben des Druckinhalts in den Shared Memory-Bereich
            for (int i = 1; i <= pages; i++) {
                shared_child_mem[i] = rand();
                printf("Anwendung erzeugt Druckauftrag mit ID %i Seite %i mit Inhalt %i\n", shm_child_id, i,
                       shared_child_mem[i]);
            }

			//Shared Memory-Id der Anwendung in Ringspeicher schreiben
            int *next_to_write = &shm_ringspeicher[NEXT_TO_WRITE];
            shm_ringspeicher[*next_to_write] = shm_child_id;

            //im Ringspeicher eins weiter zählen
            (*next_to_write)++;
            if (*next_to_write == 5) {
                *next_to_write = 0;
            }

            //Schreibzugriff abgeben
            signal_sem(semid_ringspeicher, SEMAPHORE_MUTEX);

			//Spooler über Druckauftrag informieren
            signal_sem(semid_ringspeicher, SEMAPHORE_FULL);

            //Shared Memory-Bereiche ausblenden
            shmdt(shm_ringspeicher);
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

	//Shared Memory-Bereich einblenden
	shm_ringspeicher = (int *) shmat(shm_ringspeicher_id, NULL, 0);

	//So lange durchlaufen bis LesePos gleich NextSchreibePos,
    //falls beide zu Beginn gleich sind, prüfen ob Warteschlange komplett voll
    while (shm_ringspeicher[NEXT_TO_READ] != shm_ringspeicher[NEXT_TO_WRITE] || get_sem(semid_ringspeicher, SEMAPHORE_EMPTY) == 0){
        int toEndID = shm_ringspeicher[shm_ringspeicher[NEXT_TO_READ]];
        shmctl(toEndID, IPC_RMID, NULL);
        shm_ringspeicher[NEXT_TO_READ]++;

		if (shm_ringspeicher[NEXT_TO_READ] == 5){
            shm_ringspeicher[NEXT_TO_READ] = 0;
        }

        //signal der Semaphore, um Endlosschleife zu verhindern
        signal_sem(semid_ringspeicher, SEMAPHORE_EMPTY);
    }

    //für feststeckende Prozessen in waits
    signal_sem(semid_druckerkommunikation, 2); //Drucker1 aus wait holen
    signal_sem(semid_druckerkommunikation, 3); //Drucker2 aus wait holen
    signal_sem(semid_druckerkommunikation, 1); //Spooler aus wait für Drucker2 holen
    signal_sem(semid_druckerkommunikation, 0); //Spooler aus wait für Drucker1 holen
    signal_sem(semid_ringspeicher, SEMAPHORE_FULL); //Spooler aus wait holen

	//Anwendungen aus waits holen
    for (int i = 0; i < children_counter; i++) {
        signal_sem(semid_ringspeicher, SEMAPHORE_EMPTY);
        signal_sem(semid_ringspeicher, SEMAPHORE_MUTEX);
    }

    //Warten auf Beenden der Kind-Prozesse
    while (wait(NULL) != -1);

    //Semaphoren löschen
    semctl(semid_ringspeicher, 0, IPC_RMID, 0);
    semctl(semid_druckerkommunikation, 0, IPC_RMID, 0);

    //Shared Memory-Bereiche ausblenden
    shmdt(shared_run_mem);
	shmdt(shm_ringspeicher);

    //Shared Memory-Bereiche löschen
    shmctl(shm_ringspeicher_id, IPC_RMID, NULL);
    shmctl(shm_drucker_id, IPC_RMID, NULL);
    shmctl(shm_run_id, IPC_RMID, NULL);

    return 0;
}

//Signalhandler zum Beenden des gesamten Programms
void sigterm_handler(int signb) {
    *shared_run_mem = 0;
}
