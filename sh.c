#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <utmpx.h>
#include <fcntl.h>
#include <pthread.h>
#include "sh.h"

User* first;
pthread_mutex_t lock;
int watching = 0;
int no_clobber = 0;

int sh( int argc, char **argv, char **envp )
{
  char *prompt = calloc(PROMPTMAX, sizeof(char));
  char *commandline = calloc(MAX_CANON, sizeof(char));
  char *command, *arg, *commandpath, *p, *pwd, *owd;
  char **args = calloc(MAXARGS, sizeof(char*));
  int uid, i, status, argsct, go = 1;
  struct passwd *password_entry;
  char *homedir;
  struct pathelement *pathlist;
  //char** aliaskey = calloc(20,sizeof(char*));
  //char** aliasval = calloc(20,sizeof(char*));
  //int aliasct = 0;
  char** history = calloc(10, sizeof(char*));
  int history_count = 0;
  int pipe = 0; // '|'
  int arrow = 0; // '>'
  int arrow_case = 0;
  int back_arrow = 0; // '<'
  int background = 0; // '&'
  //const char star[] = "*";
  uid = getuid();
  password_entry = getpwuid(uid);               /* get passwd info */
  homedir = password_entry->pw_dir;		/* Home directory to start
						  out with*/

  if ( (pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
  {
    perror("getcwd");
    exit(2);
  }
  owd = calloc(strlen(pwd) + 1, sizeof(char));
  memcpy(owd, pwd, strlen(pwd));
  prompt[0] = ' '; prompt[1] = '\0';

  /* Put PATH into a linked list */
  pathlist = get_path();

  while ( go )
  {
     /* print your prompt */
     printf("%s", prompt);
     printf("[%s]>",owd);

     /* get command line and process */

     // read line
     fgets(commandline, MAX_CANON, stdin);

     // update history
     if(history_count < 10){
       // printf("%s\n", commandline);
       history[history_count] = malloc(MAX_CANON * sizeof(char));
       strcpy(history[history_count], commandline);
       history_count++;
     }
     else{
       for(int i = 9; i > 0; i--){
         history[i] = history[i-1];
       }
       history[0] = commandline;
     }
     // prompt = commandline;

     // split into command/args
     command = strtok(commandline, " ");
	   args[0] = command;
     argsct = 1;
     while( ((arg = strtok(NULL, " ")) != NULL) && argsct < MAXARGS){
   	     // strcat(arg, "\0");
	     args[argsct] = arg;
	     argsct++;
     }
     strtok(args[argsct-1], "\n");
	 if(strcmp(args[argsct-1], "\n") == 0){
	 	args[argsct-1] = NULL;
	 	argsct = argsct - 1;
	 }

     


     // exit
	 if(strcmp(command, "exit") == 0){
		 printf("Executing built-in %s\n", command);
	     go = 0;
	 }
   // where
	 else if(strcmp(command, "where") == 0){
		printf("Executing built-in %s\n", command);
	 	if(argsct < 2){
	 		printf("where: Too few arguments.");
	 	}
	 	else{
	 		int i;
	 		for(i = 1; i < argsct; i++){
	 			printf("%s\n", where(args[i], pathlist));
	 		}
	 	}
	 }
   //which
	 else if(strcmp(command, "which") == 0){
     printf("Executing built-in %s\n", command);
     if(argsct < 2){
       printf("which: Too few arguments.\n");
     }
     else{
       int i;
       for(i = 1; i < argsct; i++){
         char* p = which(args[i], pathlist);
         printf("%s\n", p);
         free(p);
       }
     }
	 }
   //cd
	 else if(strcmp(command, "cd") == 0){
		printf("Executing built-in %s\n", command);
    char* tmp = owd;
    if(argsct < 2){
      pwd = tmp;
      owd = homedir;
      chdir(owd);
    }
    else if(argsct > 2){
      printf("cd: Too many arguments.\n");
    }
    else if(strcmp(args[1],"-")==0){
      owd = pwd;
      pwd = tmp;
      chdir(owd);
    }
    else{
      int dirchanged = chdir(args[1]);
      if(dirchanged == 0){
        owd = args[1];
        pwd = tmp;
      }
      else{
        printf("%s: No such file or directory\n", args[1]);
      }
    }
    owd = getcwd(NULL, PATH_MAX+1);
	 }
   //pwd
	 else if(strcmp(command, "pwd") == 0){
		printf("Executing built-in %s\n", command);
    printf("%s\n",owd);
	 }
	 else if(strcmp(command, "list") == 0){
		printf("Executing built-in %s\n", command);
    struct dirent *dirstream;
    DIR *dir;
    if(argsct < 2){
      dir = opendir(owd);
      while((dirstream = readdir(dir)) != NULL){
        printf("%s\n",dirstream->d_name);
      }
    }
    else{
      for(int i = 1; i<argsct; i++){
        dir = opendir(args[i]);
        if(dir == NULL){
          printf("cannot access '%s':\n", args[i]);
        }
        else{
          printf("%s:\n", args[i]);
          while((dirstream = readdir(dir)) != NULL){
            printf("%s\n",dirstream->d_name);
          }
          printf("\n");
        }
      }
    }
    //struct dirent *curdir = opendir(path)
	 }
	 else if(strcmp(command, "pid") == 0){
		printf("Executing built-in %s\n", command);
    pid_t pid = getpid();
    printf("pid: %d\n",pid);
	 }
	 else if(strcmp(command, "kill") == 0){
		printf("Executing built-in %s\n", command);
    if(argsct < 2){
      printf("kill: Too few arguments.\n");
    }
    else if(argsct == 2){
      pid_t pid = atoi(args[1]);
      int killed = kill(pid,SIGTERM);
      if(killed != 0){
        printf("kill: invalid process.\n");
      }
    }
    else if(argsct == 3){
      char* sig = args[1];
      if(sig[0] == '-'){
        char* newsig = (sig+1);
        int signum = atoi(newsig);
        pid_t pid = atoi(args[2]);
        int killed = kill(pid,signum);
        if(killed !=0){
          printf("kill: invalid process or signal.\n");
        }
      }
      else{
        printf("kill: Arguments should be jobs or process id's.\n");
      }
    }
    else if(argsct > 3){
      printf("kill: Too many arguments.\n");
    }
	 }
	 else if(strcmp(command, "prompt") == 0){
		printf("Executing built-in %s\n", command);
    if(argsct < 2){
      printf("prompt: Too few arguments.\n");
    }
    else if(argsct > 2){
      printf("prompt: Too many arguments.\n");
    }
    else if(argsct == 2){
      prompt = args[1];
    }
	 }
	 else if(strcmp(command, "printenv") == 0){
		printf("Executing built-in %s\n", command);
    if(argsct == 1){
      printenv(envp);
    }
    else if(argsct == 2){
      char* result = getenv(args[1]);
      if(result != NULL){
        printf("%s\n", result);
      }
    }
    else if(argsct > 2){
      printf("printenv: Too many arguments.\n");
    }
	 }

   else if(strcmp(command, "watchuser") == 0){
    printf("Executing built-in %s\n", command);
     if(argsct == 1){
       printf("watchuser: Too few arguments.\n");
     }
     else if(argsct == 2){
       watchuser(args[1],1);
     }
     else if(argsct == 3){
       if(strcmp(args[2],"off")== 0){
         watchuser(args[1],0);
       }
       else{
         printf("watchuser: %s is not a valid argument.\n", args[1]);
       }
     }
     else if(argsct>3){
       printf("watchuser: Too many arguments.\n");
     }
   }
   /*
   // Alias not completed

	 else if(strcmp(command, "alias") == 0){
		printf("Executing built-in %s\n", command);
    if(argsct == 1){
      for(int i = 0; i < aliasct; i++){
        printf("alias %s='%s'\n", aliaskey[i],aliasval[i]);
      }
    }
    else if(argsct == 2){
      char* tmp = calloc(MAX_CANON,sizeof(char));
      //strtok
    }
    else if(argsct > 2){
      for(int i = 1; i < argsct; i++){
        aliasct++;
      }
    }
	 }
   */
	 else if(strcmp(command, "history") == 0){
		printf("Executing built-in %s\n", command);
    if(argsct == 1){
      for(int i = 0; i < history_count; i++){
        printf("%s\n", history[i]);
      }
    }
    else if(argsct == 2){
      int count = atoi(args[1]);
      printf("%d\n", count);
      if(count >= 0 && count <= history_count){
        for(int i = 0; i < count; i++){
          printf("%s\n", history[i]);
        }
      }
      else{
        printf("Usage: history [# number of events].\n");
      }
    }
    else if(argsct > 2){
      printf("history: Too many arguments.\n");
    }
	 }
	 else if(strcmp(command, "setenv") == 0){
		printf("Executing built-in %s\n", command);
    if(argsct == 1){
      printenv(envp);
    }
    else if(argsct == 2){
      int set = setenv(args[1], "", 1);
      if(set == -1){
        perror("setenv:");
      }
    }
    else if(argsct == 3){
      int set = setenv(args[1], args[2], 1);
      if(set == -1){
        perror("setenv:");
      }
    }
    else if(argsct > 3){
      printf("setenv: Too many arguments");
    }
	 }
     else if(strcmp(command, "noclobber") == 0)
     {
         printf("Executing built-in %s\n", command);
         if(argsct == 1){
           if(no_clobber == 0)
           {
             putenv("noclobber=1");   
             no_clobber = 1;
           }
           else if(no_clobber == 1)
           {
             putenv("noclobber=0");   
             no_clobber = 0;
           }
         }
         else
         {
            printf("noclobber: Too many arguments."); 
         }
     }
	 else{
	 	int process_found = 0;
	 	struct pathelement* tmp = pathlist;
	 	pid_t tpid;
	 	while(tmp != NULL && process_found == 0){
	 		char* path = malloc(1000 * sizeof(char));
	 		strcpy(path,tmp->element);
	 		// strcat(path, "/");
	 		// strcat(path, command);
	 		strcat(path, "/");
	 		strcat(path, command);
            // check args for special characters
            for(int i = 0; i < argsct; i++){
               if(args[i][0] == '>'){
                   arrow = i;
                   if(strcmp(args[i], ">") == 0){
                     arrow_case = 1;
                   }
                   else if(strcmp(args[i], ">>") == 0){
                     arrow_case = 2;
                   }
                   else if(strcmp(args[i], ">&") == 0){
                     arrow_case = 3;
                   }
                   else if(strcmp(args[i], ">>&") == 0){
                     arrow_case = 4;
                   }
               }
               if(args[i][0] == '<'){
                   back_arrow = i;
               }
               if(args[i][0] == '&'){
                   background = i;
                   if(strcmp(args[i], "&>") == 0){
                     arrow_case = 3;
                     background = 0;
                   }
               }
               if(args[i][0] == '|'){
                   pipe = i;
               }
            }
            
            if(background > 0){
                args[background] = NULL;
                argsct -= 1;
            }

	 		if (access(path, F_OK) == 0){
	 			process_found = 1;
				printf("Executing %s\n", path);
	 			pid = fork();
	 			int child_status;

				if(pid == -1){ // ERROR
					perror("fork error");
					exit(EXIT_FAILURE);
				}
	 			else if(pid == 0){ // child
                    int run = 1;

                    if(signal(SIGINT, sig_handler_sh) == SIG_ERR){
                      perror("Can't catch signal");
                    }
                    if(arrow != 0){
                        char* filename = args[arrow+1];
                        
                        int new = 0;
                    
                        struct stat st;
                    
                        if(stat(filename, &st) == 0){
                            new = 0;
                        }
                        
                        
                        //Overwrite
                        if(arrow_case == 1){
                            if(no_clobber == 1 && new == 0){
                               printf("%s: File exists.\n", filename);
                               run = 0;
                            }
                            else
                            {
                              int file = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 666);
                              close(STDOUT_FILENO);
                              dup(file);
                              close(file);
                            }
                        }
                        else if(arrow_case == 2)
                        {
                            if(no_clobber == 1 && new == 1){
                                printf("%s: File does not exist\n", filename);
                                run = 0;
                            }
                            else
                            {
                                int file = open(filename, O_WRONLY|O_CREAT|O_APPEND, 666);
                                close(STDOUT_FILENO);
                                dup(file);
                                close(file);
                            }
                        }
                        else if(arrow_case == 3)
                        {
                            if(no_clobber == 1 && new == 0){
                              printf("%s: File exists.", filename);
                              run = 0;
                            }
                            else
                            {
                                int file = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 666);
                                close(STDOUT_FILENO);
                                dup(file);
                                close(STDERR_FILENO);
                                dup(file);
                                close(file);
                            }
                        }
                        else if(arrow_case == 4){
                            if(no_clobber == 1 && new == 1){
                                printf("%s: File does not exist\n", filename);
                                run = 0;
                            }
                            else
                            {
                                int file = open(filename, O_WRONLY|O_CREAT|O_APPEND, 666);
                                close(STDOUT_FILENO);
                                dup(file);
                                close(STDERR_FILENO);
                                dup(file);
                                close(file);
                            }
                        }
                        else if(back_arrow){
                            if(stat(filename, &st) == -1){
                                printf("Error opening %s\n", filename);
                                run = 0;
                            }
                            else
                            {
                                int file = open(filename, O_RDONLY);
                                close(STDIN_FILENO);
                                dup(file);
                                close(file);
                            }
                        }
                    
                        for(int i = arrow; i < argsct; i++){
                          args[i] = NULL;
                        }
                    }
                    if(run){
	 				    if(execve(path, args, envp) == -1){
	 				    	printf("%s: Command not found.\n", command);
	 				    }
                    }
	 			}
	 			else{ // parent
	 				// wait for child to finish
                    if(background > 0){
	 				  while(tpid != pid){
	 				  	tpid = waitpid(pid, &child_status, WNOHANG);
					  }
                    }
                    else
                    {
	 				  while(tpid != pid){
	 				  	tpid = waitpid(pid, &child_status, 0);
                      }
                    }
	 			}
	 		}
	 		tmp = tmp->next;
	 		free(path);
	 	}
	 	if(process_found == 0){
	 		fprintf(stderr, "%s: Command not found.\n", args[0]);
	 	}

  	 }
	 // clear out args
	 for(int i = 0; i < MAXARGS; i++){
	 	args[i] = NULL;
	 }
     // clear out special characters
     pipe = 0;
     arrow = 0;
     arrow_case = 0;
     back_arrow = 0;
     background = 0;
	}
  for(int i = 0; i < history_count; i++){
    free(history[i]);
  }
  freeUsers(first);
  free(history);
	free(prompt);
	free(commandline);
	free(args);

	return 0;
}


char *where(char *command, struct pathelement *pathlist )
{
  int pathfound = 0;
	if(strcmp(command, "exit") == 0){
		return "exit: shell built-in command.";
    pathfound = 1;
	}
	else if(strcmp(command, "which") == 0){
		return "which: shell built-in command.";
    pathfound = 1;
	}
	else if(strcmp(command, "where") == 0){
		return "where: shell built-in command.";
    pathfound = 1;
  }

	else if(strcmp(command, "cd") == 0){
		return "cd: shell built-in command.";
    pathfound = 1;
	}
	else if(strcmp(command, "pwd") == 0){
		return "pwd: shell built-in command.";
    pathfound = 1;
	}
	else if(strcmp(command, "list") == 0){
		return "ls: shell built-in command.";
    pathfound = 1;
	}
	else if(strcmp(command, "pid") == 0){
		return "pid: shell built-in command.";
    pathfound = 1;
	}
	else if(strcmp(command, "kill") == 0){
		return "kill: shell built-in command.";
    pathfound = 1;
	}
	else if(strcmp(command, "prompt") == 0){
		return "prompt: shell built-in command.";
    pathfound = 1;
	}
	else if(strcmp(command, "printenv") == 0){
		return "printenv: shell built-in command.";
    pathfound = 1;
	}
	else if(strcmp(command, "alias") == 0){
		return "alias: shell built-in command.";
    pathfound = 1;
	}
	else if(strcmp(command, "history") == 0){
		return "history: shell built-in command.";
    pathfound = 1;
	}
	else if(strcmp(command, "setenv") == 0){
		return "setenv: shell built-in command.";
    pathfound = 1;
	}
	else{
		struct pathelement* tmp = pathlist;
		while(tmp != NULL){
			char* path = malloc(1000 * sizeof(char));
			strcpy(path,tmp->element);
			strcat(path, "/");
			strcat(path, command);
			if (access(path, F_OK) == 0){
				printf("%s\n", path);
        pathfound = 1;
			}
			tmp = tmp->next;
			free(path);
		}
	}
  if(!pathfound){
    printf("%s: Command not found.\n", command);
  }
  return "";
} /* which() */

char *which(char *command, struct pathelement *pathlist )
{
  char* path = malloc(100 * sizeof(char));
  if(strcmp(command, "exit") == 0){
    strcpy(path,"exit: shell built-in command.");
		return path;
	}
	else if(strcmp(command, "which") == 0){
    strcpy(path,"which: shell built-in command.");
    return path;
	}
	else if(strcmp(command, "where") == 0){
    strcpy(path,"where: shell built-in command.");
    return path;
	}
	else if(strcmp(command, "cd") == 0){
    strcpy(path,"cd: shell built-in command.");
    return path;
	}
	else if(strcmp(command, "pwd") == 0){
    strcpy(path,"pwd: shell built-in command.");
    return path;
	}
	else if(strcmp(command, "list") == 0){
    strcpy(path,"list: shell built-in command.");
    return path;
	}
	else if(strcmp(command, "pid") == 0){
    strcpy(path,"pid: shell built-in command.");
    return path;
	}
	else if(strcmp(command, "kill") == 0){
    strcpy(path,"kill: shell built-in command.");
    return path;
	}
	else if(strcmp(command, "prompt") == 0){
    strcpy(path,"prompt: shell built-in command.");
    return path;
	}
	else if(strcmp(command, "printenv") == 0){
    strcpy(path,"printenv: shell built-in command.");
    return path;
	}
	else if(strcmp(command, "alias") == 0){
    strcpy(path,"alias: shell built-in command.");
    return path;
	}
	else if(strcmp(command, "history") == 0){
    strcpy(path,"exit: shell built-in command.");
    return path;
	}
	else if(strcmp(command, "setenv") == 0){
    strcpy(path,"exit: shell built-in command.");
    return path;
	}
	else{
		struct pathelement* tmp = pathlist;
		while(tmp != NULL){
			char* path = malloc(1000 * sizeof(char));
			strcpy(path,tmp->element);
			// strcat(path, "/");
			// strcat(path, command);
			strcat(path, "/");
			strcat(path, command);
			if (access(path, F_OK) == 0){
				return path;
			}
			tmp = tmp->next;
			free(path);
		}
	}

  strcpy(path,"");
	return path;
} /* where() */


void printenv ( char **envp )
{
  int i = 0;
  while(envp[i] != NULL){
    printf("%s\n", envp[i]);
    i += 1;
  }
}

void sig_handler_sh(int signal)
{
  if(signal == SIGINT){
    kill(pid, SIGINT);
  }
}

void* watch_user_thread(void* arg)
{
  struct utmpx *up;
  setutxent();			/* start at beginning */
  int count = 0;
  char* name = (char *) arg;
  while(count  < 20){
    while (up = getutxent())	/* get an entry */
    {
      if ( up->ut_type == USER_PROCESS )	/* only care about users */
      {
        pthread_mutex_lock(&lock);   
        if(userLogin(first, name))
        {
          printf("%s has logged on %s from %s\n", up->ut_user, up->ut_line, up ->ut_host);
        }
        pthread_mutex_unlock(&lock);
      }
    }
  count++;  
  sleep(1);
  }
}

void watchuser(char* name, int off){
  User tmp;
  pthread_t watch_thread;
  struct utmpx *up;

  if(pthread_mutex_init(&lock, NULL) != 0)
  {
      perror("\n mutex init has failed.\n");
  }

  pthread_mutex_lock(&lock);
  addUser(first, name); 
  pthread_mutex_unlock(&lock);

  if(off)
  {
    pthread_cancel(watch_thread);
    pthread_join(watch_thread, NULL);
    watching = 0;
  }
  else if(watching)
  {
    pthread_cancel(watch_thread);
    pthread_join(watch_thread, NULL);
    pthread_create(watch_thread, NULL, watch_user_thread, name); 
  }
  else
  {
    watching = 1;
    pthread_create(watch_thread, NULL, watch_user_thread, name); 
  }

}
