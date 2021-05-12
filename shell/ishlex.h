/*--------------------------------------------------------------------*/
/* ishlex.h                                                          */
/* NAME: Bereket Assefa                                               */
/* ID: 20170844                                                       */
/*--------------------------------------------------------------------*/
#ifndef ISHLEX_INCLUDED
#define ISHLEX_INCLUDED

/* Free token pvItem.  pvExtra is unused. */
void freeToken(void *pvItem, void *pvExtra);

/* Lexically analyze string pcLine.  Populate oTokens with the
   tokens that pcLine contains.  Return 1 (TRUE) if successful, or
   0 (FALSE) otherwise.  In the latter case, oTokens may contain
   tokens that were discovered before the error. The caller owns the
   tokens placed in oTokens. */

/* lexLine() uses a DFA approach.  It "reads" its characters from
   pcLine. */
int lexLine(const char *pcLine, DynArray_T oTokens,char *executable);
#endif
