//  Partie interface du module shm_fifo

//  Ce module permet de mettre en place, une gestion de file de pid en mémoire
//    partager

#ifndef SHM_FIFO__H
#define SHM_FIFO__H

#include <sys/types.h>

typedef struct fifo_t fifo_t;

extern fifo_t *shm_fifo_init(const char *name, unsigned int length);
extern fifo_t *shm_fifo_open(const char *name, unsigned int length);
extern int shm_fifo_enqueue(fifo_t *f, pid_t p);
extern pid_t shm_fifo_dequeue(fifo_t *f);
extern int shm_fifo_dispose(fifo_t **f);
extern int shm_fifo_unlink(const char *name);

#endif
