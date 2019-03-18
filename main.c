#include "sh.h"
#include <signal.h>
#include <stdio.h>

void sig_handler(int signal);

int main( int argc, char **argv, char **envp )
{
  if(signal(SIGINT, sig_handler) == SIG_ERR){
    perror("Can't catch signal");
  }
  if(signal(SIGTSTP, sig_handler) == SIG_ERR){
    perror("Can't catch signal");
  }
  if(signal(SIGTERM, sig_handler) == SIG_ERR){
    perror("Can't catch signal");
  }

  return sh(argc, argv, envp);
}

void sig_handler(int signal)
{
  if(signal == SIGINT){
    //printf("caught SIGINT");
  }
  if(signal == SIGTSTP){
    //printf("caught SIGTSTP");
  }
  if(signal == SIGTERM){
    //printf("caught SIGTERM, ignoring");
  }
}
