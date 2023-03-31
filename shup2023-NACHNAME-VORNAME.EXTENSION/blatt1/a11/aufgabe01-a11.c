//
// Created by erdnaskram on 24.03.23.
//
#include <stdio.h>

int main(int argc, char *argv[]) {
    for (int i = 0; i < argc; i++)
        //%i wegen int für 'i', %s für das arg
        // aufruf dann einfach alle argumente mit leerzeichen trennen
        printf("%i: %s\n", i, argv[i]);

}