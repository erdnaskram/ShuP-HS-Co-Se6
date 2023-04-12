#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


int main (int argc, char *argv[]){
	int child = fork();

	//Child
	if(child == 0){
		int value = 0;
		value = execl("HelloWorld", "HelloWorld", "", "", (char*) NULL);
		if(value != -1)
			exit(0);
		else
			exit(1);
	}//Parent
	else{
		int status;
		pid_t pid = wait(&status);
		printf("Kindprozess mit pid %i beendet mit Status %i\n", (int)pid, (int)status);
	}
}

