//
// Created by PRAKTIKANT on 21.04.2023.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

int main(int argc, char *argv[]) {
    // Überprüfen, ob zwei Argumente übergeben wurden
    if (argc != 3) {
        printf("Usage: %s <Anzahl Schleifendurchläufe> <Seed>\n", argv[0]);
        return 1;
    }

    // Anzahl Schleifendurchläufe und Seed aus den Argumenten auslesen
    int n = atoi(argv[1]);
    int seed = atoi(argv[2]);

    // Shared Memory-Bereich erzeugen
    int shm_id = shmget(IPC_PRIVATE, sizeof(int), 0777 | IPC_CREAT);
    if (shm_id < 0) {
        printf("Fehler beim Erzeugen des Shared Memory-Bereichs\n");
        return 1;
    }
    printf("Shared Memory ID: %d\n", shm_id);
    // Zufallsgenerator mit Seed initialisieren
    srand(seed);

    // Kindprozess erzeugen
    int pid = fork();
    printf("PID: %d\n", pid);
    if (pid == -1) {
        printf("Fehler beim Erzeugen des Kindprozesses\n");
        return 1;
    } else if (pid == 0) {
        // Kindprozess liest Zufallszahlen aus Shared Memory
        printf("KINDPROZESS\n");
        int *shared_mem = shmat(shm_id, NULL, 0);
        printf("%d\n", n);
        for (int i = 0; i < n; i++) {

            //Problemzeile
            printf("Kindprozess: %d\n", *shared_mem);


            printf("%d\n", i);
        }
        shmdt(shared_mem);
    } else {
        // Vaterprozess schreibt Zufallszahlen in Shared Memory
        int *shared_mem = shmat(shm_id, NULL, 0);
        for (int i = 0; i < n; i++) {
            *shared_mem = rand();
            printf("Vaterprozess: %d\n", *shared_mem);
        }
        shmdt(shared_mem);

        // Shared Memory-Bereich freigeben
        shmctl(shm_id, IPC_RMID, NULL);
    }

    return 0;
}