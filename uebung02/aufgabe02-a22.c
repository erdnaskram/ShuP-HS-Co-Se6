//
// Created by joni134 on 24.03.23.
//
#include <stdio.h>
#include <unistd.h>

int main (int argc, char *argv[]) {
	int mainpid = getpid();
	printf("Main-Process-ID: %i\n", mainpid);
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
			printf("Do C1/C2\n");
			break;
		case 3:
			// C3
			printf("Do C3\n");
			break;
		default:
			// Parent
			printf("Do Parent\n");
	}
}
