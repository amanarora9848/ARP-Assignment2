#! /usr/bin/bash

gcc src/processA.c -lncurses -lbmp -lm -o bin/processA
gcc src/processB.c -lncurses -lbmp -lm -o bin/processB
gcc src/circle.c -lbmp -lm -o bin/circle
gcc src/master.c -o bin/master

echo "Compiled."