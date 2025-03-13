#ifndef _UTILS_H
#define _UTILS_H

#include <stdlib.h>

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

#define CHECK_T(status, msg)                                                 \
  if (0 != (status))   {                                                     \
    fprintf(stderr, "pthread erreur : %s avec erreur n°%d\n", msg, status);  \
    exit (EXIT_FAILURE);                                                     \
  }

#ifdef DEBUG
    #define DEBUG_PRINT(msg, ...) do {                                      \
        printf(msg, ##__VA_ARGS__);                                         \
    } while (0)
#else
    #define DEBUG_PRINT(msg, ...) // Ne fait rien si DEBUG n'est pas défini
#endif