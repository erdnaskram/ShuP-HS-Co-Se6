//
// Created by erdnaskram on 24.03.23.
//
#include <stdio.h>
#include <stdlib.h>
int main (int argc, char *argv[]) {
    int anzName = atoi(argv[1]);
    char *namen[5] = {0};

    printf("NAMEN DÜRFEN NICHT LÄNGER ALS 20 ZEICHEN SEIN !!11!11!\n");
    for (int i = 0;i < anzName; i++) {
        printf("Geben sie den %i. Namen ein\n", i+1);
        scanf("%s", namen[i]);
        printf("Danke :) \n");
    }

    printf("\n\nUmgedrehte Namen:\n");
    for (int i = anzName-1;i >= 0; i--) {
        printf("%i. Name : %s\n", i+1, namen[i]);
    }
}