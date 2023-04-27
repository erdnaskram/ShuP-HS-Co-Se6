/*
 * aufgabe03-a33.c
 * Copyright (C) 2023 Zoe Schulz <zoe.schulz@mail.de>
 *
 */

#include "aufgabe03-a33.h"


int main(){

	// 	Shared Memory-Bereich erzeugen
	int shm_id =shmget(IPC_PRIVATE, 5*sizeof(int), 0777 | IPC_CREAT);
	if(shm_id < 0){
		printf("Fehler beim Erzeugen des Shared Memory-Bereichs\n");
		return 1;
	}

	int spooler = fork();

	if(spooler == -1){
		printf("Fehler beim Erzeugen des Spooler-Prozesses");
		return 1;
	}else if(spooler == 0){
		printf("Spoooooooler");
		return 0;
	}

	int drucker1 = fork();

	if(drucker1 == -1){
		printf("Fehler beim Erzeugen des Drucker1-Prozesses");
		return 1;
	}else if(drucker1 == 0){
		printf("Drucker1");
		return 0;
	}


	int drucker2 = fork();

	if(drucker2 == -1){
		printf("Fehler beim Erzeugen des Drucker2-Prozesses");
		return 1;
	}else if(drucker2 == 0){
		printf("Drucker2");
		return 0;
	}


	int pid;
	srand(time(NULL));

	//TODO: Schleife abbrechen
	while(1){
		sleep(rand()%5);
		pid = fork();
		if(pid == -1){
			printf("Fehler beim Erzeugen des Kindprozesses\n");
			return 1;
		}else if(pid == 0){
			//sychronisieren
			int *shared_mem = (int*) shmat(shm_id, NULL, 0);
			*shared_mem = rand(); //VORSICHT Pointer-Aritmetik
			return 0; //sonst gehen Kinder auch wieder in Schleife
		}
	}


	int status;
	while(1){
		wait(&status);
		if(&status == NULL)
			break;
	}

	// Shared Memory-Bereich freigen
	shmctl(shm_id, IPC_RMID, NULL);

	return 0;


}
