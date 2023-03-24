//
// Created by joni134 on 24.03.23.
//
#include <stdio.h>
#include <unistd.h>

int main (int argc, char *argv[]) {
	fork();
	printf("Process-ID: %i\n",getpid());
	printf("Parent-Process-ID: %i\n",getppid());
	while(1)
		sleep(1);
}

// Ergebnisse:
// - Wenn Parent-Prozess zuerst beendet: PS zeigt Parent-Prozess nicht mehr an (Kommentar "+1 getötet" am Ende), Kind-Prozess wird normal angezeigt.
// - Wenn Kind-Prozess zuerst beendet: PS zeigt Kind-Prozess mit Kommentar "defunct" an, es werden aber weiterhin beide Prozesse angezeigt.
