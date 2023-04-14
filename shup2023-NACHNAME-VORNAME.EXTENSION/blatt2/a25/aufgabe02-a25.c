//
// Created by #Studis19# on 14.04.23.
//

#include <stdio.h>
#include <stdlib.h>


void setColor(int color) {
    printf("\033[0;%dm", color);
}

int main(int argc, char *argv[]) {
    setColor(32);
    printf("%s@HOST", getenv("USER"), getenv("PWD"));
    setColor(0);
    printf(":");
    setColor(34);
    printf("%s ", getenv("PWD"));
    setColor(0);
    printf("$ ");



}