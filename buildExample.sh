#!/bin/bash

gcc -g -Wall -O3 -c esa.c -o esa.o
gcc -g -Wall -O3 esa.o esaExample.c -o esaExample
./esaExample 

