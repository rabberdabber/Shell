/*--------------------------------------------------------------------*/
/* ishsyn.c                                                           */
/* NAME: Bereket Assefa                                               */
/* ID: 20170844                                                       */
/*--------------------------------------------------------------------*/


#include "dynarray.h"
#include "ishlex.h"
#include "ishsyn.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

enum COMMAND_SPECIFIER{COMMAND_NAME ,COMMAND_ARG,COMMAND_IN_FILE,
		       COMMAND_OUT_FILE};

enum {MAX_LINE_SIZE = 1024};

enum DFA_STATE {START,ARG,IN_FILE,OUT_FILE,PIPE};

enum {FALSE, TRUE};



/* This function adds each commands into a dynamic array */
static int Add_To_Command_Page(struct Command_Page *CommandPtr,
   char *Command_Str, enum COMMAND_SPECIFIER Command_Specifier){

  switch(Command_Specifier){

     case COMMAND_NAME:
        CommandPtr-> Command_Name = Command_Str;
        break;

     case COMMAND_ARG:
       if(!DynArray_add(CommandPtr->oArgs,Command_Str)){
	  DynArray_free(CommandPtr->oArgs);
	  return FALSE;
       }
       break;

     case COMMAND_IN_FILE:
       if(CommandPtr->Command_In_File != NULL)
         return	FALSE;
       CommandPtr->Command_In_File =  Command_Str;
       break;

     case COMMAND_OUT_FILE:
       if(CommandPtr->Command_Out_File != NULL)
         return	FALSE;
       CommandPtr->Command_Out_File = Command_Str;
       break;
       
  }
  return TRUE;
}
/* This function initializes command pages that will be pointed by the dynamic array */
static struct Command_Page *Init_Command(DynArray_T oCommandTable){

  struct Command_Page *Page = (struct Command_Page*) calloc((size_t) 1,
					      sizeof(struct Command_Page));
  if(Page == NULL){
     fprintf(stderr, "Cannot allocate memory\n");
     return NULL;
  }
  Page->oArgs = DynArray_new(0);
  if(Page->oArgs == NULL){
    fprintf(stderr,"Cannot allocate memory\n");
    free(Page);
    return NULL;
  }
  if(!DynArray_add(oCommandTable,Page)){
     fprintf(stderr, "Cannot allocate memory\n");
     free(Page);
     return NULL;
  }
  return Page;
}
/* return the state given the character */
static void Analyze_Token(DynArray_T oTokens,int iTokNum,
			   enum DFA_STATE *state){
  
  enum TokenType myTok = ((struct Token *)oTokens->ppvArray[iTokNum])->eType;
  if(((struct Token *)oTokens->ppvArray[iTokNum])->is_special == 0)
    *state = ARG;
  else if(myTok == STDOUT_REDIRECT_TOKEN)
    *state = OUT_FILE;
  else if(myTok == STDIN_REDIRECT_TOKEN)
    *state = IN_FILE;
  else if(myTok == TOKEN_PIPE)
    *state = PIPE;
}

void freePage(void *pvItem, void *pvExtra)

/* Free Page pvItem.  pvExtra is unused. */

{
   struct Command_Page *Page = (struct Command_Page*)pvItem;
   DynArray_free(Page->oArgs);
   free(Page);
}


/* This function parses tokens and add command info to commandtable */
DynArray_T Parse(DynArray_T oTokens,char *executable){

  assert(executable != NULL);
  int iTokenNum = 0;
  enum DFA_STATE state = START,next_state = START,before_state = START;
  int start_flag = 1,set_right_end = 0,stdout_redirected = 0;;
  struct Command_Page *CommandPtr = NULL;
  
  if(oTokens == NULL)
  {
    fprintf(stderr,"%s: The input dynamic array is not valid!\n",executable);
    return NULL;
  }
  DynArray_T oCommandTable = DynArray_new(0);
  if (oCommandTable == NULL)
  {
     fprintf(stderr, "%s: Cannot allocate memory\n",executable);
     return NULL;
  }
  while(iTokenNum < oTokens->iLength)
  {
    switch(state)
    {

      case START:
	if(((struct Token *)oTokens->ppvArray[iTokenNum])->is_special == 1){
	  if(!iTokenNum)
	    fprintf(stderr,"%s: Missing command name\n",executable);
	  else
	    fprintf(stderr, "%s: Pipe or redirection destination is not specified\n",executable);
	  DynArray_map(oCommandTable, freePage, NULL);
	  DynArray_free(oCommandTable);
	  return NULL;
	}
	if((CommandPtr = Init_Command(oCommandTable)) == NULL){
	  fprintf(stderr,"%s: Cannot allocate memory\n",executable);
	  DynArray_map(oCommandTable,freePage, NULL);
          DynArray_free(oCommandTable);
	  return NULL;
	}
	if(!Add_To_Command_Page(CommandPtr,((struct Token *)oTokens->ppvArray[iTokenNum])->pcValue,COMMAND_NAME)){
	  DynArray_map(oCommandTable, freePage, NULL);
          DynArray_free(oCommandTable);
	  return NULL;
	}
	if(set_right_end){
	  CommandPtr->right_end = 1;
	}
	set_right_end = 0;
	start_flag = 0;
	break;

      case ARG:
	if(before_state == IN_FILE){
	  // if standard input is redirected again
	  if(!Add_To_Command_Page(CommandPtr,((struct Token *)oTokens->ppvArray[iTokenNum])->pcValue,COMMAND_IN_FILE)){
	    fprintf(stderr,"%s: Multiple redirection of standard input\n",executable);
	    DynArray_map(oCommandTable, freePage, NULL);
            DynArray_free(oCommandTable);
            return NULL;
          }
	}
	else if(before_state == OUT_FILE){
	  if(!Add_To_Command_Page(CommandPtr,((struct Token *)oTokens->ppvArray[iTokenNum])->pcValue,COMMAND_OUT_FILE)){
	    fprintf(stderr,"%s: Multiple redirection of standard output\n",executable);
	      DynArray_map(oCommandTable, freePage, NULL);
              DynArray_free(oCommandTable);
	      return NULL;
           }
	}
	// if not a redirected file
	else if(!Add_To_Command_Page(CommandPtr,((struct Token *)oTokens->ppvArray[iTokenNum])->pcValue,COMMAND_ARG)){
          fprintf(stderr,"%s: Insertion of arguments failed\n",executable);
          DynArray_map(oCommandTable, freePage, NULL);
          DynArray_free(oCommandTable);
	  return NULL;
        }
	break;

      case IN_FILE:
	if(CommandPtr->right_end){
	    fprintf(stderr,"%s: Multiple redirection of standard input\n",executable);
            DynArray_map(oCommandTable, freePage, NULL);
            DynArray_free(oCommandTable);
            return NULL;
	}
	// no tokens after redirection
	if(iTokenNum == oTokens->iLength-1){
          fprintf(stderr,"%s: Standard input redirection without file name\n",executable);
          DynArray_map(oCommandTable, freePage, NULL);
          DynArray_free(oCommandTable);
	  return NULL;
        }
	break;

      case OUT_FILE:
        //no tokens after redirection
        if(iTokenNum == oTokens->iLength-1){
          fprintf(stderr,"%s: Standard output redirection without file name\n",executable);
          DynArray_map(oCommandTable, freePage, NULL);
          DynArray_free(oCommandTable);
	  return NULL;
        }
	      stdout_redirected  = 1;
        break;
	
      case PIPE:
	 if(stdout_redirected){
	    fprintf(stderr ,"%s: Multiple redirection of standard output\n",executable);
            DynArray_map(oCommandTable, freePage, NULL);
            DynArray_free(oCommandTable);
            return NULL;
        }

        if(iTokenNum == oTokens->iLength-1){
	  fprintf(stderr,"%s: Pipe or redirection destination is not specified\n",executable);
          DynArray_map(oCommandTable, freePage, NULL);
          DynArray_free(oCommandTable);
	  return NULL;
         }
      	CommandPtr->left_end = 1;
      	set_right_end = 1;
        next_state = state = before_state = START;
      	start_flag = 1;
        break;
    }
     if(iTokenNum < oTokens->iLength - 1 && start_flag != 1){
       Analyze_Token(oTokens,++iTokenNum, &next_state);
       if(((struct Token *)oTokens->ppvArray[iTokenNum])->is_special == 1){
	  if(state == PIPE){
	     fprintf(stderr,"%s: Pipe or redirection destination is not specified\n",executable);
             DynArray_map(oCommandTable, freePage, NULL);
             DynArray_free(oCommandTable);
	  }   
	  else if(state == IN_FILE){
	    fprintf(stderr,"%s: Standard input redirection without file name\n",executable);
             DynArray_map(oCommandTable, freePage, NULL);
             DynArray_free(oCommandTable);
	  }
          else if(state == OUT_FILE){
	    fprintf(stderr,"%s: Standard output redirection without file name\n",executable);
	     DynArray_map(oCommandTable, freePage, NULL);
             DynArray_free(oCommandTable);
             return NULL;
          }
       }
       before_state = state;
       state = next_state;
     }
    else
       iTokenNum++;
 }
    return oCommandTable;
}

