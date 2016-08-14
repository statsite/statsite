#include "libcompat.h"
#include <stdio.h>

#define INITIAL_SIZE 16
#define DELIMITER '\n'

ssize_t getline(char **lineptr, size_t *n, FILE *stream)
{
     ssize_t written = 0;
     int character;

     if(*lineptr == NULL || *n < INITIAL_SIZE)
     {
          free(*lineptr);
          *lineptr = (char *)malloc(INITIAL_SIZE);
          *n = INITIAL_SIZE;
     }

     while( (character = fgetc(stream)) != EOF)
     {
          written += 1;
          if(written >= *n)
          {
               *n = *n * 2;
               *lineptr = realloc(*lineptr, *n);
          }

          (*lineptr)[written-1] = character;

          if(character == DELIMITER)
          {
               break;
          }
     }

     (*lineptr)[written] = '\0';

     return written;
}
