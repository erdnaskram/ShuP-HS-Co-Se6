#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

int main (int argc, char *argv[]) {
	printf("Diese Lösung wurde erstellt von Christoph Marks\n");

	//Prüfen auf Aufrufparameter
	if(argc<5) {
		printf("Geben Sie mindestens 4 Aufrufparameter an!\n");
		return 1;
	}

	int mainpid = getpid();
	printf("Parent-Process-ID: %i\n", mainpid);
	printf("Aufrufparameter 1: %s\n",argv[1]);

	int pidC1,pidC2,pidC3,childNumber;
	pidC1 = fork();

	//bei Fehler Abbrechen
	if(pidC1 == -1){
		fprintf(stderr, "Fehler bei Erzeugen des 1. Kindprozesses");
		return(1);
	}
	if(pidC1 != 0) {
		//Vaterprozess (Nach C1, erstelle C2)
		pidC2 = fork();

		if(pidC2 == -1){
			fprintf(stderr, "Fehler bei Erzeugen des 2. Kindprozesses");
			return(1);
		}
		if(pidC2 != 0) {
			//Vaterprozess (Nach C2, erstelle C3)
			pidC3 = fork();

			if(pidC3 == -1){
				fprintf(stderr, "Fehler bei Erzeugen des 3. Kindprozesses");
				return(1);
			}
			if(pidC3 != 0) {
				//Vaterprozess (Final, setze childNumber)
				childNumber = 0;
			} else {
				//Kindprozess 3
				childNumber = 3;
			}
		} else {
			//Kindprozess 2
			childNumber = 2;
		}
	} else {
		//Kindprozess 1
		childNumber = 1;
	}

	//Aufgaben an Kindprozesse zuweisen
	switch(childNumber) {
		case 1:
		case 2:
			//C1+C2
			printf("Aufrufparameter %i: %s\n",childNumber+1,argv[childNumber+1]);
			printf("Child%i-Process-ID: %i\n",childNumber, getpid());
			while(1);
			break;
		case 3:
			//C3
			printf("Aufrufparameter %i: %s\n",childNumber+1,argv[childNumber+1]);
			printf("Child%i-Process-ID: %i\n",childNumber, getpid());
			sleep(1);
			exit(2);
			break;
		default:
			// Parent
			sleep(2);
			kill(pidC1,15); // 15 = SIGTERM!
			kill(pidC2,9); // 9 = SIGKILL!
			kill(pidC3,9); // 9 = SIGKILL!

			int status;
			int pid;
			for (int n = 3; n > 0; n--){
				pid = wait(&status);
				printf("Kindprozess mit Prozess-ID %i endete mit Status %i\n", pid, status);
			}
		}
		exit(0);
}