#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/mman.h>
#include "utils.h"
#include "bytecode.h"
#include "vm.h"

void interpret_bc (Program* p) {
  printf("Interpreting Bytecode Program:\n");
  print_prog(p);
  printf("\n");
}

