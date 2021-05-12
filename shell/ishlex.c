/*--------------------------------------------------------------------*/
/* ishlex.c                                                           */
/* NAME: Bereket Assefa                                               */
/* ID: 20170844                                                       */
/*--------------------------------------------------------------------*/

#include "dynarray.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*--------------------------------------------------------------------*/

enum {MAX_LINE_SIZE = 1024};

enum {FALSE, TRUE};



 void freeToken(void *pvItem, void *pvExtra)

/* Free token pvItem.  pvExtra is unused. */

{
   struct Token *psToken = (struct Token*)pvItem;
   free(psToken->pcValue);
   free(psToken);
}


static struct Token *makeToken(enum TokenType eTokenType,
			       char *pcValue,int is_special)

/* Create and return a Token whose type is eTokenType and whose
   value consists of string pcValue.  Return NULL if insufficient
   memory is available.  The caller owns the Token. */

{
   struct Token *psToken;

   psToken = (struct Token*)malloc(sizeof(struct Token));
   if (psToken == NULL)
      return NULL;

   psToken->eType = eTokenType;
   psToken->is_special = is_special;

   psToken->pcValue = (char*)malloc(strlen(pcValue) + 1);
   if (psToken->pcValue == NULL)
   {
      free(psToken);
      return NULL;
   }

   strcpy(psToken->pcValue, pcValue);

   return psToken;
}

/*--------------------------------------------------------------------*/

int lexLine(const char *pcLine, DynArray_T oTokens,char *executable)

/* Lexically analyze string pcLine.  Populate oTokens with the
   tokens that pcLine contains.  Return 1 (TRUE) if successful, or
   0 (FALSE) otherwise.  In the latter case, oTokens may contain
   tokens that were discovered before the error. The caller owns the
   tokens placed in oTokens. */

/* lexLine() uses a DFA approach.  It "reads" its characters from
   acLine. */

{
  enum LexState {STATE_START,STATE_IN_WORD,STATE_IN_PIPE,
		 STATE_IN_STDIN_REDIRECT, STATE_IN_STDOUT_REDIRECT};

   enum LexState eState = STATE_START;

   int iLineIndex = 0;
   int iValueIndex = 0;
   int count_quotes = 0;
   char c;
   char acValue[MAX_LINE_SIZE];
   struct Token *psToken;

   assert(pcLine != NULL);
   assert(oTokens != NULL);

   for (;;)
   {
      /* "Read" the next character from pcLine. */
      c = pcLine[iLineIndex++];

      switch (eState)
      {
         case STATE_START:
            if ((c == '\n') || (c == '\0'))
               return TRUE;
            else if (isspace(c))
            {
               eState = STATE_START;
            }
            else if (isalnum(c) || c == '\"')
	    {
	       if(!isalnum(c))
	         count_quotes++;
               else
		 acValue[iValueIndex++] = c;
               eState = STATE_IN_WORD;
            }
	    else if(c == '|')
	    {
	       acValue[iValueIndex++] = c;
	       eState = STATE_IN_PIPE;
	    }
	    else if(c == '>')
	    {
	      acValue[iValueIndex++] = c;
	      eState = STATE_IN_STDOUT_REDIRECT;
	    }
	    else if(c == '<')
	    {
	      acValue[iValueIndex++] = c;
              eState = STATE_IN_STDIN_REDIRECT;	      
	    }
            else
            {
               acValue[iValueIndex++] = c;
               eState = STATE_IN_WORD;
            }
            break;

         case STATE_IN_PIPE:
	    /* Create a pipe token. */
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_PIPE, acValue,1);
               if (psToken == NULL)
               {
                  fprintf(stderr, "Cannot allocate memory\n");
                  return FALSE;
               }
               if (! DynArray_add(oTokens, psToken))
               {
		 fprintf(stderr, "%s: Cannot allocate memory\n",executable);
                  return FALSE;
               }
               iValueIndex = 0;
	       
            if ((c == '\n') || (c == '\0'))
            {
               return TRUE;
            }
            else if (isspace(c))
            {
               eState = STATE_START;
            }
            else if (isalnum(c) || c == '\"')
	    {
	       if(!isalnum(c))
	         count_quotes++;
               else
		 acValue[iValueIndex++] = c;
               eState = STATE_IN_WORD;
            }
	    else if(c == '|')
	    {
	       acValue[iValueIndex++] = c;
	       eState = STATE_IN_PIPE;
	    }
	    else if(c == '>')
	    {
	      acValue[iValueIndex++] = c;
	      eState = STATE_IN_STDOUT_REDIRECT;
	    }
	    else if(c == '<')
	    {
	      acValue[iValueIndex++] = c;
              eState = STATE_IN_STDIN_REDIRECT;	      
	    }
            else
            {
	      acValue[iValueIndex++] = c;
              eState = STATE_IN_WORD;
            }

            break;
	    
	  case STATE_IN_WORD:
	    if ((c == '\n') || (c == '\0'))
            {
	       if(count_quotes & 1){
		 fprintf(stderr,"%s: unmatched quote\n",executable);
		   return FALSE;
	       }
	       /* Create a word token. */
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_WORD, acValue,0);
               if (psToken == NULL)
               {
		 fprintf(stderr, "%s: Cannot allocate memory\n",executable);
                  return FALSE;
               }
               if (! DynArray_add(oTokens, psToken))
               {
		 fprintf(stderr, "%s: Cannot allocate memory\n",executable);
                  return FALSE;
               }
 
               return TRUE;
            }
	    else if (isalnum(c) || c == '\"')
	    {
	       if(!isalnum(c))
	         count_quotes++;
               else
		 acValue[iValueIndex++] = c;
               eState = STATE_IN_WORD;
            }
	    // if quote is is unmatched stay in word state
	    else if(count_quotes & 1){
	       acValue[iValueIndex++] = c;
               eState = STATE_IN_WORD;
	    }
            else if (isspace(c))
            {
	       /* Create a word token. */
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_WORD, acValue,0);
               if (psToken == NULL)
               {
		 fprintf(stderr, "%s: Cannot allocate memory\n",executable);
                  return FALSE;
               }
               if (! DynArray_add(oTokens, psToken))
               {
		 fprintf(stderr, "%s: Cannot allocate memory\n",executable);
                  return FALSE;
               }
               iValueIndex = 0;
               eState = STATE_START;
            }
	    else if(c == '|')
	    {
	       /* Create a word token. */
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_WORD, acValue,0);
               if (psToken == NULL)
               {
		 fprintf(stderr, "%s: Cannot allocate memory\n",executable);
                  return FALSE;
               }
               if (! DynArray_add(oTokens, psToken))
               {
		 fprintf(stderr, "%s: Cannot allocate memory\n",executable);
                  return FALSE;
               }
               iValueIndex = 0;

	       acValue[iValueIndex++] = c;
	       eState = STATE_IN_PIPE;
	    }
	    else if(c == '>')
	    {
	       /* Create a word token. */
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_WORD, acValue,0);
               if (psToken == NULL)
               {
		 fprintf(stderr, "%s: Cannot allocate memory\n",executable);
                  return FALSE;
               }
               if (! DynArray_add(oTokens, psToken))
               {
		 fprintf(stderr, "%s: Cannot allocate memory\n",executable);
                  return FALSE;
               }
               iValueIndex = 0;

	       acValue[iValueIndex++] = c;
	       eState = STATE_IN_STDOUT_REDIRECT;
	    }
	    else if(c == '<')
	    {
	      /* Create a word token. */
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_WORD, acValue,0);
               if (psToken == NULL)
               {
		 fprintf(stderr, "%s: Cannot allocate memory\n",executable);
                  return FALSE;
               }
               if (! DynArray_add(oTokens, psToken))
               {
		 fprintf(stderr, "%s: Cannot allocate memory\n",executable);
                  return FALSE;
               }
               iValueIndex = 0;
 
	       acValue[iValueIndex++] = c;
               eState = STATE_IN_STDIN_REDIRECT;	      
	    }
            else
            {
	      /* if it is another characters */
               acValue[iValueIndex++] = c;
               eState = STATE_IN_WORD;
            }
            break;
	    
  
         case STATE_IN_STDOUT_REDIRECT:
           
            /* Create a stdout redirect token. */
            acValue[iValueIndex] = '\0';
            psToken = makeToken(STDOUT_REDIRECT_TOKEN, acValue,1);
            if (psToken == NULL)
            {
	      fprintf(stderr, "%s: Cannot allocate memory\n",executable);
	       return FALSE;
            }
            if (! DynArray_add(oTokens, psToken))
            {
	      fprintf(stderr, "%s: Cannot allocate memory\n",executable);
               return FALSE;
            }
            iValueIndex = 0;
	    if ((c == '\n') || (c == '\0'))
            {
	       return TRUE;
            }
	    else if (isspace(c))
            {
               eState = STATE_START;
            }
            else if (isalnum(c) || c == '\"')
	    {
	       if(!isalnum(c))
	         count_quotes++;
               else
		 acValue[iValueIndex++] = c;
               eState = STATE_IN_WORD;
            }
	    else if(c == '|')
	    {
	       acValue[iValueIndex++] = c;
	       eState = STATE_IN_PIPE;
	    }
	    else if(c == '>')
	    {
	      acValue[iValueIndex++] = c;
	      eState = STATE_IN_STDOUT_REDIRECT;
	    }
	    else if(c == '<')
	    {
	      acValue[iValueIndex++] = c;
              eState = STATE_IN_STDIN_REDIRECT;	      
	    }
            else
            {
	      acValue[iValueIndex++] = c;
              eState = STATE_IN_WORD;
            }

            break;
	 case STATE_IN_STDIN_REDIRECT:
           
            /* Create a stdin redirect token. */
            acValue[iValueIndex] = '\0';
            psToken = makeToken(STDIN_REDIRECT_TOKEN, acValue,1);
            if (psToken == NULL)
            {
	      fprintf(stderr, "%s: Cannot allocate memory\n",executable);
	       return FALSE;
            }
            if (! DynArray_add(oTokens, psToken))
            {
	      fprintf(stderr, "%s: Cannot allocate memory\n",executable);
               return FALSE;
            }
            iValueIndex = 0;
	    if ((c == '\n') || (c == '\0'))
            {
	       return TRUE;
            }
	    else if (isspace(c))
            {
               eState = STATE_START;
            }
            else if (isalnum(c) || c == '\"')
	    {
	       if(!isalnum(c))
	         count_quotes++;
               else
		 acValue[iValueIndex++] = c;
               eState = STATE_IN_WORD;
            }
	    else if(c == '|')
	    {
	       acValue[iValueIndex++] = c;
	       eState = STATE_IN_PIPE;
	    }
	    else if(c == '>')
	    {
	      acValue[iValueIndex++] = c;
	      eState = STATE_IN_STDOUT_REDIRECT;
	    }
	    else if(c == '<')
	    {
	      acValue[iValueIndex++] = c;
              eState = STATE_IN_STDIN_REDIRECT;	      
	    }
            else
            {
	       acValue[iValueIndex++] = c;
               eState = STATE_IN_WORD;
            }

            break;
	    
	default:
	    assert(FALSE);
           
      }
   }
}


