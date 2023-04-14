//
// Created by joni134 on 24.03.23.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

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

	//bei Fehler Abbrechen
	if(pc1 == -1)
		return(1);

	if(pc1 != 0) {
		// Parent (Nach C1, erstelle C2)
		pc2 = fork();

		if(pc2 == -1)
			return(1);
		if(pc2 != 0) {
			// Parent (Nach C2, erstelle C3)
			pc3 = fork();

			if(pc3 == -1)
				return(1);
			if(pc3 != 0) {
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
			while(1);
			break;
		case 3:
			// C3
			printf("Aufrufparameter %i: %s\n",processFunction+1,argv[processFunction+1]);
			printf("Child%i-Process-ID: %i\n",processFunction, getpid());
			sleep(1);
			exit(2);
			break;
		default:
			// Parent
			sleep(2);
			kill(pc1,15); // 15 = SIGTERM!
			kill(pc2,9); // 9 = SIGKILL!
			kill(pc3,9); // 9 = SIGKILL!

			int status,exitsignal;
			pid_t pid;
			for (int n = 3; n > 0; n--){
				pid = wait(&status);
				if(WIFEXITED(status)) {
					exitsignal = WEXITSTATUS(status);
					printf(
						"Child-Prozess mit Process-ID %i endete mit Status %i (von wait() gesetzter Status: %i)\n",
						(int)pid,
						exitsignal,
						(int) status
					);
				}
				if(WIFSIGNALED(status)) {
					exitsignal = WTERMSIG(status);
					printf(
						"Child-Prozess mit Process-ID %i endete durch Signal %i (von wait() gesetzter Status: %i)\n",
						(int)pid,
						exitsignal,
						(int) status
					);
				}
			}

			exit(0);
	}
}