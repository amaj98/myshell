
#include "get_path.h"

typedef struct User
{ 
  char* name;
  int on;
  struct User* next; 
} User;

int pid;
int sh( int argc, char **argv, char **envp);
char *which(char *command, struct pathelement *pathlist);
char *where(char *command, struct pathelement *pathlist);
void list ( char *dir );
void printenv(char **envp);
void sig_handler_sh(int signal);

#define PROMPTMAX 32
#define MAXARGS 10
