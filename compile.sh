#! /usr/bin/bash

gcc src/processA.c -lncurses -lbmp -lpthread -lrt -lm -o bin/processA
gcc src/processB.c -lncurses -lbmp -lpthread -lrt -lm -o bin/processB
gcc src/master.c -o bin/master

echo "Compiled."