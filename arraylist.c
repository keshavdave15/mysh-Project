#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Include string.h for strdup
#include "arraylist.h"

#ifndef DEBUG
#define DEBUG 0
#endif

void al_init(arraylist_t *L, unsigned size)
{
    L->data = malloc(size * sizeof(char*)); // Change data type to char*
    L->length = 0;
    L->capacity = size;
}

void al_destroy(arraylist_t *L)
{
    // Free memory for each string stored in the arraylist
    for (unsigned i = 0; i < L->length; i++) {
        free(L->data[i]);
    }
    free(L->data);
}

unsigned al_length(arraylist_t *L)
{
    return L->length;
}

void al_push(arraylist_t *L, char *item) // Change parameter type to char*
{
    if (L->length == L->capacity)
    {
        L->capacity *= 2;
        char **temp = realloc(L->data, L->capacity * sizeof(char*)); // Change data type to char**
        if (!temp)
        {
            // for our own purposes, we can decide how to handle this error
            // for more general code, it would be better to indicate failure to our caller
            fprintf(stderr, "Out of memory!\n");
            exit(EXIT_FAILURE);
        }
        L->data = temp;
        if (DEBUG)
            printf("Resized array to %u\n", L->capacity);
    }
    // Allocate memory for the new string and copy the content
    L->data[L->length] = strdup(item);
    if (L->data[L->length] == NULL) {
        // Handle memory allocation failure
        fprintf(stderr, "Out of memory!\n");
        exit(EXIT_FAILURE);
    }
    L->length++;
}

// No change needed for al_pop since it's not used for strings
// You can leave it as it is
