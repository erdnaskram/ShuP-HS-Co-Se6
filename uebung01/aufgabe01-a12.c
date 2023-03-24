//
// Created by erdnaskram on 24.03.23.
//
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char *argv[]) {
    int anzName = atoi(argv[1]);
    char **namen;
    namen = (char **) malloc(anzName * sizeof(char *));

    for (int i = 0;i < anzName; i++) {
        namen[i] = (char *) malloc(100 * sizeof(char));
        printf("Geben Sie den %i. Namen ein\n",i+1);
        fgets(namen[i],100,stdin);
    }

    printf("\n\nUmgedrehte Namen:\n");
    for(int i=anzName-1;i>=0;i--) {
        printf("%i. Name: %s\n",i+1,namen[i]);
	free(namen[i]);
    }
    free(namen);
}
