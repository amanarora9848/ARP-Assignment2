#! /usr/bin/bash

gcc -fdiagnostics-color=always src/processA.c -lncurses -lbmp -lpthread -lrt -lm -o bin/processA
gcc -fdiagnostics-color=always src/processB.c -lncurses -lbmp -lpthread -lrt -lm -o bin/processB
gcc -fdiagnostics-color=always src/master.c -o bin/master

echo "Compiled."