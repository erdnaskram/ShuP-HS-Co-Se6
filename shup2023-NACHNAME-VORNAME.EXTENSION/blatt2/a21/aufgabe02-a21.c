//
// Created by joni134 on 24.03.23.
//
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    fork();
    printf("Process-ID: %i\n", getpid());
    printf("Parent-Process-ID: %i\n", getppid());
    while (1);
}

// Ergebnisse:
// - Wenn Parent-Prozess zuerst beendet: PS zeigt Parent-Prozess nicht mehr an (Kommentar "+1 getötet" am Ende), Kind-Prozess wird normal angezeigt.
// - Wenn Kind-Prozess zuerst beendet: PS zeigt Kind-Prozess mit Kommentar "defunct" an, es werden aber weiterhin beide Prozesse angezeigt.




// Kind-Prozess zuerst beendet

// root@jh-shup:~/shup/blatt2# ./a.out &
// [1] 8622
// root@jh-shup:~/shup/blatt2# Process-ID: 8622
// Parent-Process-ID: 8481
// Process-ID: 8623
// Parent-Process-ID: 8622
// ps
//    PID TTY          TIME CMD
//   8481 pts/3    00:00:00 bash
//   8622 pts/3    00:00:00 a.out
//   8623 pts/3    00:00:00 a.out
//   8624 pts/3    00:00:00 ps
//root@jh-shup:~/shup/blatt2# kill -9 8623
//root@jh-shup:~/shup/blatt2# ps
//    PID TTY          TIME CMD
//   8481 pts/3    00:00:00 bash
//   8622 pts/3    00:00:00 a.out
//   8623 pts/3    00:00:00 a.out <defunct>
//   8625 pts/3    00:00:00 ps
// root@jh-shup:~/shup/blatt2# kill -9 8622
// root@jh-shup:~/shup/blatt2# ps
//     PID TTY          TIME CMD
//    8481 pts/3    00:00:00 bash
//    8626 pts/3    00:00:00 ps
// [1]+  Getötet                ./a.out




// Parent-Prozess zuerst beendet

// root@jh-shup:~/shup/blatt2# kill -9 8613
// -bash: kill: (8613) - Kein passender Prozess gefunden
// root@jh-shup:~/shup/blatt2# ./a.out &
// [1] 8617
// root@jh-shup:~/shup/blatt2# Process-ID: 8617
// Parent-Process-ID: 8481
// Process-ID: 8618
// Parent-Process-ID: 8617
// ps
//     PID TTY          TIME CMD
//    8481 pts/3    00:00:00 bash
//    8617 pts/3    00:00:00 a.out
//    8618 pts/3    00:00:00 a.out
//    8619 pts/3    00:00:00 ps
// root@jh-shup:~/shup/blatt2# kill -9 8617
// root@jh-shup:~/shup/blatt2# ps
//     PID TTY          TIME CMD
//    8481 pts/3    00:00:00 bash
//    8618 pts/3    00:00:00 a.out
//    8620 pts/3    00:00:00 ps
// [1]+  Getötet                ./a.out
// root@jh-shup:~/shup/blatt2# kill -9 8618
// root@jh-shup:~/shup/blatt2# ps
//     PID TTY          TIME CMD
//    8481 pts/3    00:00:00 bash
//    8621 pts/3    00:00:00 ps

