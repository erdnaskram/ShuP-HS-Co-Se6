#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

void sigtermhandler(int);

int counter = 0;

int main (int argc, char *argv[]){
	printf("Programm gestartet\n");
	printf("Bei 3-maligem SIGTERM oder SIGINT beendet sich das Programm\n");

	signal(SIGTERM, sigtermhandler); //15
	signal(SIGINT, sigtermhandler); //2

	while(1);


}

void sigtermhandler(int signb){
	counter++;
	printf("Signal: %i eingetroffen\n", signb);
	if(counter == 3){
		printf("Programm beendet sich\n");
		exit(5);
	}
}
