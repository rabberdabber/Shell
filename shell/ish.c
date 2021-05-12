/*--------------------------------------------------------------------*/
/* ish.c                                                              */
/* NAME: Bereket Assefa                                               */
/* ID: 20170844                                                       */
/*--------------------------------------------------------------------*/

#define _DEFAULT_SOURCE
#include "dynarray.h"
#include "ishlex.h"
#include "ishsyn.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

enum {MAX_LINE_SIZE = 1024};
enum {NUMBER_OF_BUILTINS = 4};
enum REDIRECTION {STDIN_REDIRECTION,STDOUT_REDIRECTION};
enum SIGINT_MODE{DEFAULT,IGNORE};
enum SIGQUIT_MODE{TERMINATE,WAIT};

static int iCount = 0;// global var for signal handling

/* Executes built-in functions and returns 1 if successful or -1 if not */
static int Execute_Builtin(int argc,const char **argv,char *executable){
    
    assert(argv != NULL && executable != NULL);
    char *Builtin_Command[] = {"setenv","unsetenv","cd","exit"};
    if(!strcmp(argv[0],Builtin_Command[0])){
        
      if(argc == 2){
            if(setenv(argv[1],"",1) == -1){
                perror(executable);
                return 0;
            }
        
      }
      else if(argc == 3){
            if(setenv(argv[1],argv[2],1) == -1){
                perror(executable);
                return 0;
            }
      }
      else{
	fprintf(stderr,"%s:  setenv takes one or two parameters\n",executable);
         return 0;
      }
      
    }
    else if(!strcmp(argv[0],Builtin_Command[1])){
        if(argc == 2){
          if(unsetenv(argv[1]) == -1){
             perror(executable);
             return 0;
          }
        }
       else{
	  fprintf(stderr,"%s:  unsetenv takes one or two parameters\n",executable);
          return 0;
       }
    }
    else if(!strcmp(argv[0],Builtin_Command[2])){
        
       if(argc == 1){
           if(chdir(getenv("HOME")) == -1){
             perror(executable);
             return 0;
         }
       }
       else if(argc == 2){
           if(chdir(argv[1]) == -1){
             perror(executable);
             return 0;
           }
       }

	    else{
	     fprintf(stderr,"%s: cd takes one parameter\n",executable);
  	     return 0;
	}
	       
    }
       	
   else if(!strcmp(argv[0],Builtin_Command[3])){
       if(argc == 1)
         exit(0);
       
       else{
         fprintf(stderr,"%s: exit does not take any parameters\n",executable);
         return 0;
       }
     
   }

  return 1;

}
/* check if the string is a builtin function return 1 if it is */	   
/*--------------------------------------------------------------------*/    
static int Is_Builtin(const char *Command_Name){
    
    assert(Command_Name != NULL);
    
    int i;
    char *Builtin_Command[] = {"setenv","unsetenv","cd","exit"};
    
    for(i = 0;i<NUMBER_OF_BUILTINS;i++){
        if(!strcmp(Command_Name,Builtin_Command[i]))
            return 1;
    }    
        return 0;
}

/* it redirects stdin and stdout accoringly */
static int redirect(struct Command_Page *CommandPtr
		    ,enum REDIRECTION redirection,const char *filename,char *executable){
  
  assert(CommandPtr != NULL && filename != NULL
	 && executable != NULL);
  int fd;
  switch(redirection){
          
      case STDIN_REDIRECTION:
          fd = open(filename,O_RDONLY);
          if(fd < 0){
              perror(executable);
              return 0;
          }
          if(dup2(fd,STDIN_FILENO) < 0){
              perror(executable);   
              if(close(fd)< 0){
                perror(executable);
                return 0;
              }

          }
          if(close(fd) < 0){
              perror(executable);
              return 0;
          }
          
          break;

      case STDOUT_REDIRECTION:
          fd = creat(filename,0600);
          if(fd < 0){
              perror(executable);
              return 0;
          }
          if(dup2(fd,STDOUT_FILENO) < 0){
              perror(executable);
              if(close(fd) < 0){
                perror(executable);
                return 0;
              }
          }
          if(close(fd) < 0){
              perror("close failed");
              return 0;
          }
          
          break;
  }
  return 1;
}
/* this function either ignores or makes signals 
   according to their default actions */
static void Handle_SigInt(enum SIGINT_MODE sig_mod){

  void (*ret) (int);

  switch(sig_mod){
    case IGNORE:
      ret = signal(SIGINT,SIG_IGN);
      assert(ret != SIG_ERR);
      break;
    case DEFAULT:
      ret = signal(SIGINT,SIG_DFL);
      assert(ret != SIG_ERR);
      break;
  }

}
/* set the global variable to zero */
static void Alarm_Handler(int iSig){
  iCount = 0;
}

/* if ctrl-\ is typed within 5 seconds exit the process */
static void Wait_SigQuit(int iSig){

  iCount++;
  
  if(iCount == 1){
    printf("\nType Ctrl-\\ again within 5 seconds to exit.\n%% ");
    fflush(stdout);
    alarm(5);
  }

  else if(iCount == 2){
    exit(0);
  }

}
/* if child, do nothing but if parent call the wait and alarm handlers */
static void Handle_SigQuit(enum SIGQUIT_MODE sig_mod){

  void (*ret) (int);

  switch(sig_mod){
    
    case TERMINATE:

      // so should terminate
      break;

    case WAIT:
      ret = signal(SIGQUIT,Wait_SigQuit);
      assert(ret != SIG_ERR);

      ret = signal(SIGALRM,Alarm_Handler);
      assert(ret != SIG_ERR);
      break;

  }


}
/* This function takes in command table and executes it
   if it is a single command without a pipe          */
static int Execute_Command(DynArray_T oCommandTable,char *executable){

    assert(oCommandTable != NULL);
    const char *in,*out;
    char * const * arg;
    char *Command_Name;
    struct Command_Page *Page = (struct Command_Page *)oCommandTable->ppvArray[0];
    Command_Name = Page->Command_Name;
    pid_t iPid;
    DynArray_T dynArg = Page->oArgs;
    if(!DynArray_addAt(dynArg,0,(void *) Command_Name)|| !DynArray_addAt(dynArg,dynArg->iLength,NULL)){
        fprintf(stderr, "%s: Cannot allocate memory\n",Command_Name);
        return 0;
    }
    arg =  (char * const *)dynArg->ppvArray;
    fflush(NULL);
    if((iPid = fork()) < 0){
        perror(Command_Name);
        return 0;
    }
    
    if(!iPid){
        Handle_SigInt(DEFAULT);
        Handle_SigQuit(TERMINATE);

    if((in = Page->Command_In_File) != NULL){
      if(!redirect(Page,STDIN_REDIRECTION,in,Command_Name))
	     return 0;
	  }
    if((out = Page->Command_Out_File) != NULL){
      if(!redirect(Page,STDOUT_REDIRECTION,out,Command_Name))
	    return 0;
    }
      execvp(arg[0],arg);
      perror(Command_Name);
      exit(EXIT_FAILURE);
    }

    else
        wait(NULL);
    
    return 1;
    
}
/* Execute the commands and this function is called if there is pipe */
static int Execute_Pipes(DynArray_T oCommandTable,char *executable){
    assert(oCommandTable != NULL);
    
    int NumOfPipes = oCommandTable->iLength - 1,Command_Num = 0;
    int fd[NumOfPipes * 2],CurrentFd = 0,i;
    const char *in,*out;
    char * const *arg;
    char *Command_Name;
    pid_t iPid;
    struct Command_Page *CommandPtr;
    DynArray_T dynArg;

    for(i = 0;i < NumOfPipes;i++){
        if(pipe(fd + 2 * i) < 0){
            perror(executable);
            return 0;
        }
    }

    while(Command_Num < NumOfPipes + 1){

        CommandPtr = (struct Command_Page *)oCommandTable->ppvArray[Command_Num];
        Command_Name = CommandPtr->Command_Name;
        dynArg= CommandPtr->oArgs;
        if( (!DynArray_addAt(dynArg,0,(void *) Command_Name) ||
             !DynArray_addAt(dynArg,dynArg->iLength,NULL))){
	   fprintf(stderr, "%s: Cannot allocate memory\n",executable);
           return 0;
        }
        arg = (char * const *)dynArg->ppvArray;
        in = CommandPtr->Command_In_File;
        out = CommandPtr->Command_Out_File;

        fflush(NULL);
        // fork another process after one terminates                                                                                                                                                       
        if((iPid = fork()) < 0){
            perror(executable);
            exit(EXIT_FAILURE);
            //check for memory leak
        }
        if(!iPid){

           Handle_SigInt(DEFAULT);
           Handle_SigQuit(TERMINATE);

           if(Command_Num == 0 || Command_Num == NumOfPipes){
              if(in != NULL)
		redirect(CommandPtr,STDIN_REDIRECTION,in,executable);
              else if(out != NULL)
		redirect(CommandPtr,STDOUT_REDIRECTION,out,executable);
          }
          if(Command_Num != 0){
             if(dup2(fd[CurrentFd - 2],STDIN_FILENO) < 0){
	         perror(executable);
                 exit(EXIT_FAILURE);
	     }
            for(i = 0;i < NumOfPipes * 2;i++){
                if(close(fd[i]) < 0){
                  perror(executable);
                  exit(EXIT_FAILURE);
                }
            }
            execvp(arg[0],arg);
            perror(Command_Name);
            exit(EXIT_FAILURE);
	  }
          if(Command_Num != NumOfPipes){
            if(dup2(fd[CurrentFd + 1],STDOUT_FILENO) < 0){
                perror(executable);
                exit(EXIT_FAILURE);

	    }
	 
            for(i = 0;i < NumOfPipes * 2;i++){
                if(close(fd[i]) < 0){
                    perror(executable);
                    exit(EXIT_FAILURE);
                }
            }
            execvp(arg[0],arg);
            perror(Command_Name);
            exit(EXIT_FAILURE);
	  }
       }
      Command_Num++;
      CurrentFd += 2;
    }

         //Parent closes files
        for(i = 0;i < NumOfPipes * 2;i++){
            if(close(fd[i]) < 0){
                perror(executable);
                return 0;
            }
        }
  
       for(i = 0;i < NumOfPipes + 1;i++){
         wait(NULL);
       }

    return 1;
 }
/* calls execute functions and implements a shell */
int main(int argc,char **argv)
    
    /* Read a line from stdin, and write to stdout each number and word                                                                                                                                       
     
    that it contains.  Repeat until EOF.  Return 0 iff successful. */
    
    {
      int iRet;
      sigset_t sSet;
      sigemptyset(&sSet);
      sigaddset(&sSet,SIGQUIT);
      sigaddset(&sSet,SIGINT);
      sigaddset(&sSet,SIGALRM);
      iRet = sigprocmask(SIG_UNBLOCK,&sSet,NULL);
      assert(iRet == 0);
     
    //block SIGINT
    Handle_SigInt(IGNORE);
    Handle_SigQuit(WAIT);
    char acLine[MAX_LINE_SIZE];
    const char **arg;
    DynArray_T oTokens,oCommandTable,dynArg;
    struct Command_Page *Page;
    char *command_name = NULL,*pName = NULL,*filename = NULL;
    int NumOfCommands;
    size_t size;
    FILE *fptr = NULL;
    char *address = getenv("HOME");
    if(address)
        filename = malloc(strlen(address) + 1);
		      
    if(filename){
      strcpy(filename,getenv("HOME"));
      size = strlen("/.ishrc") + strlen(filename);
     }
    pName = malloc(size + 1);
    if(filename && pName){
      strcpy(pName,filename);
      strcat(pName,"/.ishrc");
      fptr = fopen(pName,"r");
      free(filename);
      free(pName);
    }

    printf("%% ");
    
    while((fptr && fgets(acLine,MAX_LINE_SIZE,fptr)) || (fgets(acLine,MAX_LINE_SIZE,stdin)))
    {
        printf("%% ");
	if(fptr){
	  printf("%s",acLine);
	  fflush(stdout);
	}
        oTokens = DynArray_new(0);
        if (oTokens == NULL)
        {
	    fprintf(stderr, "%s: Cannot allocate memory\n",argv[0]);
	    continue;
        }
	
        lexLine(acLine, oTokens,argv[0]);
        oCommandTable = Parse(oTokens,argv[0]);
	if(oCommandTable)
           NumOfCommands = oCommandTable->iLength;
	
	if(oCommandTable && NumOfCommands > 0){
           Page = (struct Command_Page *)oCommandTable->ppvArray[0];
           dynArg= Page->oArgs;
           command_name = Page->Command_Name;
	}
        if(oCommandTable && command_name && Is_Builtin(command_name)){
          if(!DynArray_addAt(dynArg,0,(void *)command_name) || !DynArray_addAt(dynArg,dynArg->iLength,NULL))
	    fprintf(stderr, "%s: Cannot allocate memory\n",argv[0]);
      	  arg =  (const char **)dynArg->ppvArray;
      	  Execute_Builtin(oTokens->iLength,arg,argv[0]); 
                
	}
        else if(oCommandTable){
  	      if(NumOfCommands == 1)
		Execute_Command(oCommandTable,argv[0]);
  		    
              else if(NumOfCommands > 1)
	        Execute_Pipes(oCommandTable,argv[0]);
        }
	
         DynArray_map(oTokens, freeToken, NULL);
	 DynArray_free(oTokens);
	 if(oCommandTable){
	   DynArray_map(oCommandTable,freePage,NULL);
           DynArray_free(oCommandTable);
	 }

	 printf("%% ");
    }

    
    if(fptr != NULL)
      fclose(fptr);
    return 0;

 }
