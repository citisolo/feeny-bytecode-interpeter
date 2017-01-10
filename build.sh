#!/bin/bash 
echo "***********************************************************************"
echo "                                                                       "
echo "                  Building ...                                         "
echo "                                                                       "
echo "***********************************************************************"
gcc src/cfeeny.c src/utils.c src/bytecode.c src/vm.c -g --std=c11  -o cfeeny 
