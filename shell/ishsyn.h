/*--------------------------------------------------------------------*/
/* ishsyn.h                                                           */
/* NAME: Bereket Assefa                                               */
/* ID: 20170844                                                       */
/*--------------------------------------------------------------------*/
#ifndef ISHSYN_INCLUDED
#define ISHSYN_INCLUDED


/* This structure holds all the information
   of the parsed information */
struct Command_Page {

  /* It points to name of the command */
  char *Command_Name;
  
  /* It stores argument list */ 
  DynArray_T oArgs;

  /* It stores input file name */
  char *Command_In_File;

  /* It stores ouput file name */
  char *Command_Out_File;

  /* It sets the bit if the Command is left end of a pipe */
  int left_end;

  /* It sets the bit if the Command is right end of a pipe */
  int right_end;

};
/* This function frees allocated pages */
void freePage(void *pvItem, void *pvExtra);

/* This function parses tokens and add command info to commandtable */
DynArray_T Parse(DynArray_T oTokens,char *executable);
#endif
