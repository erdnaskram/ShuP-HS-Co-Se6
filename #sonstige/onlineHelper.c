//
// Created by PRAKTIKANT on 21.04.2023.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/wait.h>

#define SEM_KEY 123456
#define SEM_NUM 2

void wait_sem(int semid, int semnum) {
     struct sembuf op;
     op.sem_num = semnum;
     op.sem_op = -1;
     op.sem_flg = 0;
     semop(semid, &op, 1);
}

void signal_sem(int semid, int semnum) {
     struct sembuf op;
     op.sem_num = semnum;
     op.sem_op = 1;
     op.sem_flg = 0;
     semop(semid, &op, 1);
}

int main() {
     int semid = semget(SEM_KEY, SEM_NUM, IPC_CREAT | 0666);
     if (semid == -1) {
           perror("semget failed");
           exit(1);
         }

     // Set initial semaphore values
     union semun {
           int val;
           struct semid_ds *buf;
           ushort *array;
         } arg;
     arg.val = 0;
     semctl(semid, 0, SETVAL, arg);
     arg.val = 0;
     semctl(semid, 1, SETVAL, arg);

     pid_t pid = fork();
     if (pid == -1) {
           perror("fork failed");
           exit(1);
         } else if (pid == 0) {
           // Child process
           printf("Child process started\n");
           wait_sem(semid, 0); // Wait for parent signal
           printf("Child process received signal from parent\n");
           printf("Enter a message: ");
           fflush(stdout);
           char msg[100];
           fgets(msg, 100, stdin);
           signal_sem(semid, 1); // Signal parent
           printf("Child process exiting\n");
           exit(0);
         } else {
           // Parent process
           printf("Parent process started\n");
           printf("Enter a message: ");
           fflush(stdout);
           char msg[100];
           fgets(msg, 100, stdin);
           signal_sem(semid, 0); // Signal child
           printf("Parent process sent signal to child\n");
           wait_sem(semid, 1); // Wait for child signal
           printf("Parent process received signal from child\n");
           printf("Parent process exiting\n");
           semctl(semid, 0, IPC_RMID, 0); // Remove semaphores
         }
     return 0;
}