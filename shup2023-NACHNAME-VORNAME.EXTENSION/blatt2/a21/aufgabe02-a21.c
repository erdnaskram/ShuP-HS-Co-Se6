#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	printf("Diese Lösung wurde erstellt von <Vorname> <Nachname>\n");

    int pid = fork();
	if(pid == -1){
		fprintf(stderr, "Fehler beim Erzeugen des Kindprozesses\n");
		return 1;
	}

    printf("Kindprozess-ID: %i\n", getpid());
    printf("Vaterprozess-ID: %i\n", getppid());
    while (1);

	return 0;
}