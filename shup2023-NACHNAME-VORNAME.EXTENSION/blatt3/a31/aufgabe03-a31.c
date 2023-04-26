//
// Created by erdnaskram on 21.04.23.
//


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sem.h>


void wait_sem(int semid, int semnum) {
    struct sembuf op; //TODO: rename to sops
    op.sem_num = semnum;
    op.sem_op = -1;
    op.sem_flg = 0;
    if (semop(semid, &op, 1) == -1) {
        perror("semop failed");
        exit(1);
    }
}

void signal_sem(int semid, int semnum) {
    struct sembuf op; //TODO: rename to sops
    op.sem_num = semnum;
    op.sem_op = 1;
    op.sem_flg = 0;
    if (semop(semid, &op, 1) == -1) {
        perror("semop failed");
        exit(1);
    }
}

int main() {
    int semid = semget(IPC_PRIVATE, 2, 0777 | IPC_CREAT); //Wieso 2?
    if (semid == -1) {
        perror("semget failed"); //TODO: Ausgabe von errno
        exit(1);
    }

    semctl(semid, 0, SETALL, 0);

    int pid = fork();
    if (pid == -1) {
        perror("fork failed");
        exit(1);
    } else if (pid == 0) {
        // Child process
        printf("Child process started\n");
        wait_sem(semid, 0); // Wait for parent signal
        printf("Child process received signal from parent\n");
        printf("Enter a message: ");
        //fflush(stdout);
        char msg[100];
        fgets(msg, 100, stdin);
        signal_sem(semid, 1); // Signal parent
        printf("Child process exiting\n");
        exit(0);
    } else {
        // Parent process
        printf("Parent process started\n");
        printf("Enter a message: ");
        //fflush(stdout);
        char msg[100];
        fgets(msg, 100, stdin);
        signal_sem(semid, 0); // Signal child
        printf("Parent process sent signal to child\n");
        wait_sem(semid, 1); // Wait for child signal
        printf("Parent process received signal from child\n");
        printf("Parent process exiting\n");
        semctl(semid, 0, IPC_RMID, 0); // Remove semaphores --> löscht nur eine
    }
    return 0;
}
