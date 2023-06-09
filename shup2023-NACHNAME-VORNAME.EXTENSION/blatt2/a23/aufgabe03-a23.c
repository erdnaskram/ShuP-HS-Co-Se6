#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

void sigtermhandler(int);

int counter = 0;

int main (int argc, char *argv[]){
	printf("Diese Lösung wurde erstellt von <Vorname> <Nachname>\n");

	printf("Programm gestartet\n");
	printf("Bei 3-maligem SIGTERM oder SIGINT beendet sich das Programm\n");

	//Signalhandler registrieren
	signal(SIGTERM, sigtermhandler);
	signal(SIGINT, sigtermhandler);

	while(1);

}

//Signalhandler
void sigtermhandler(int signb){
	counter++;
	printf("Signal: %i eingetroffen\n", signb);
	if(counter == 3){
		printf("Programm beendet sich\n");
		exit(5);
	}
}
