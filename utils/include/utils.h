#ifndef _UTILS_H
#define _UTILS_H

#include <stdlib.h>

#define MAX_ITEMS_NAME_SIZE 50

#define CHECK_ERROR(val1,val2,msg)   if (val1==val2) \
                                    { perror(msg); \
                                        exit(EXIT_FAILURE); }

#define CHECK_S(status, msg)                                                 \
  if (SEM_FAILED == (status))   {                                                     \
    fprintf(stderr, "Sémaphore nommée erreur : %s\n", msg);                           \
    exit (EXIT_FAILURE);                                                     \
  }

#define CHECK_MAP(status, msg)                                                   \
  if (MAP_FAILED == (status)) {                                                    \
      perror(msg);                                                         \
      exit(EXIT_FAILURE);                                                  \
}

#define CHECK(status, msg)                                                   \
    if (-1 == (status)) {                                                    \
        perror(msg);                                                         \
        exit(EXIT_FAILURE);                                                  \
    }

#define CHECK_0(status, msg)                                                   \
    if ((status) <0) {                                                    \
        perror(msg);                                                         \
        exit(EXIT_FAILURE);                                                  \
    }

#endif
