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
    printf("%s@HOST:%s$ ", getenv("USER"), getenv("PWD"));



}