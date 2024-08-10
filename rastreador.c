#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>

#include <syscall.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#if __WORDSIZE == 64
#define REG(reg) reg.orig_rax
#else
#define REG(reg) reg.orig_eax
#endif

//__________________________________________________________________________
/**
 * Esta estructura define un nodo conformado por el nombre del system call 
 * generado mediante el procesamiento de los archivos /usr/include/x86_64-linux-gnu/asm/unistd_64.h y /usr/include/x86_64-linux-gnu/asm/unistd_34.h
 * ademas de un contador que siempre inicia en 0
 * */
typedef struct xcall {  char *sys_name; long counter; } struct_xcall;

/** 
 * Este comando define una arreglo de estructuras que carga los systemcalls de los archivos system_call_x32.h y system_call_x64.h
 * dependiendo de si estamos en un sistema de 32 bits o de 64 bits
 * */
struct_xcall syscall_table[] = {
# ifdef __i386__
#  include "system_call_x32.h"
# else
#  include "system_call_x64.h"
# endif
};
//__________________________________________________________________________


int main(int argc, char* argv[]) {   
  pid_t child;

  if (argc == 1) {
    exit(0);
  }

  char* chargs[argc];
  int i = 0;

  while (i < argc - 1) {
    chargs[i] = argv[i+1];
    i++;
  }
  chargs[i] = NULL;

  child = fork();
  if(child == 0) {
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    execvp(chargs[0], chargs);
  } else {
    int status;

    while(waitpid(child, &status, 0) && ! WIFEXITED(status)) {
      struct user_regs_struct regs; 
      ptrace(PTRACE_GETREGS, child, NULL, &regs);
      //__________________________________________________________________________
      /**
       * Cada vez que se encuentra un nuevo systemcall, se debe incrementar el contador en 1
       * se tiene reemplazar el valor de "REG(regs)" por el numero de system call encontrado
       **/
      syscall_table[REG(regs)].counter =  syscall_table[REG(regs)].counter + 1;
      //__________________________________________________________________________
      ptrace(PTRACE_SYSCALL, child, NULL, NULL);
    }
  //__________________________________________________________________________
  /**
   * Esta seccion itera sobre el arreglo de estructuras buscando los systemcalls cuyo contador sea diferente de 0
   * en caso de encontrar uno de ellos, imprimer el nombre del system call y el contador
   **/ 
  for (i = 0; i < sizeof(syscall_table)/sizeof(syscall_table[0]) ; ++i)
  {
   	if( syscall_table[i].counter != 0){
           printf("%s - %ld \n", syscall_table[i].sys_name, syscall_table[i].counter);
        } 
  }
  //__________________________________________________________________________
  }
  return 0;
}
