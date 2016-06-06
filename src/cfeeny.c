#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "utils.h"
#include "bytecode.h"
#include "vm.h"

int main (int argc, char** argvs) {
  //Check number of arguments
  if(argc != 2){
    printf("Expected 2 arguments to commandline.\n");
    exit(-1);
  }

  //Read in bytecode
  Program* p = load_bytecode(argvs[1]);
  interpret_bc(p);
  //  initvm(link_program(p));
  //  runvm();
  return 0;
}


