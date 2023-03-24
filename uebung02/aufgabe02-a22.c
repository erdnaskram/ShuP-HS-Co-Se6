//
// Created by joni134 on 24.03.23.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int main (int argc, char *argv[]) {
	if(argc<5) {
		printf("Geben Sie mindestens 4 Aufrufparameter an!\n");
		return 1;
	}

	int mainpid = getpid();
	printf("Parent-Process-ID: %i\n", mainpid);
	printf("Aufrufparameter 1: %s\n",argv[1]);

	int pc1,pc2,pc3,processFunction;
	pc1 = fork();
	if(getpid() == mainpid) {
		// Parent (Nach C1, erstelle C2)
		pc2 = fork();
		if(getpid() == mainpid) {
			// Parent (Nach C2, erstelle C3)
			pc3 = fork();
			if(getpid() == mainpid) {
				// Parent (Final, setze prozessFunktion)
				processFunction = 0;
			} else {
				// Child 3
				processFunction = 3;
			}
		} else {
			// Child 2
			processFunction = 2;
		}
	} else {
		// Child 1
		processFunction = 1;
	}

	switch(processFunction) {
		case 1:
		case 2:
			// C1+C2
			printf("Aufrufparameter %i: %s\n",processFunction+1,argv[processFunction+1]);
			printf("Child%i-Process-ID: %i\n",processFunction, getpid());
			while(1) { sleep(1); }
			break; // ToDo: Rest implementieren!
		case 3:
			// C3
			printf("Aufrufparameter %i: %s\n",processFunction+1,argv[processFunction+1]);
			printf("Child%i-Process-ID: %i\n",processFunction, getpid());
			sleep(1);
			exit(2);
			break; // ToDo: Rest implementieren!
		default:
			// Parent
			sleep(2);
			kill(pc1,15); // 15 = SIGTERM!
			kill(pc2,9); // 9 = SIGKILL!
			kill(pc3,9); // 9 = SIGKILL!
			// ToDo: Endestatus für C1, C2, C3 holen und ausgeben
			exit(0);
	}
}
