// shm_fifo.c : partie implantation de l'interface shm_fifo.h

#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif
#define _XOPEN_SOURCE 500

#include "shm_fifo.h"
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>

struct fifo_t {
  sem_t mutex;
  sem_t empty;
  sem_t full;
  unsigned int len;
  size_t curr;
  size_t curw;
  pid_t fifo[];
};

fifo_t *shm_fifo_init(const char *name, unsigned int length) {
  int shm_fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  if (shm_fd == -1) {
    return NULL;
  }
  fifo_t *d;
  if (ftruncate(shm_fd, (off_t) (sizeof *d + length * sizeof(pid_t))) != 0) {
    return NULL;
  }
  d = mmap(NULL, sizeof *d + length * sizeof(pid_t), PROT_READ | PROT_WRITE,
          MAP_SHARED, shm_fd, 0);
  if (d == MAP_FAILED) {
    return NULL;
  }
  if (sem_init(&d->mutex, 1, 1) != 0) {
    return NULL;
  }
  if (sem_init(&d->full, 1, 0) != 0) {
    return NULL;
  }
  if (sem_init(&d->empty, 1, length) != 0) {
    return NULL;
  }
  d->curr = 0;
  d->curw = 0;
  d->len = length;
  return d;
}

fifo_t *shm_fifo_open(const char *name, unsigned int length) {
  int shm_fd = shm_open(name, O_RDWR, S_IRUSR | S_IWUSR);
  if (shm_fd == -1) {
    return NULL;
  }
  fifo_t *d;
  d = mmap(NULL, sizeof *d + length * sizeof(pid_t), PROT_READ | PROT_WRITE,
          MAP_SHARED, shm_fd, 0);
  if (d == MAP_FAILED) {
    return NULL;
  }
  return d;
}

int shm_fifo_enqueue(fifo_t *f, pid_t p) {
  if (sem_wait(&f->empty) != 0) {
    return -1;
  }
  if (sem_wait(&f->mutex) != 0) {
    return -1;
  }
  f->fifo[f->curw] = p;
  f->curw = (f->curw + 1) % f->len;
  if (sem_post(&f->mutex) != 0) {
    return -1;
  }
  if (sem_post(&f->full) != 0) {
    return -1;
  }
  return 0;
}

pid_t shm_fifo_dequeue(fifo_t *f) {
  if (sem_wait(&f->full) != 0) {
    return (pid_t) -1;
  }
  if (sem_wait(&f->mutex) != 0) {
    return (pid_t) -1;
  }
  pid_t p = f->fifo[f->curr];
  f->curr = (f->curr + 1) % f->len;
  if (sem_post(&f->mutex) != 0) {
    return (pid_t) -1;
  }
  if (sem_post(&f->empty) != 0) {
    return (pid_t) -1;
  }
  return p;
}

int shm_fifo_dispose(fifo_t **f) {
  if (*f == NULL) {
    return 0;
  }
  if (sem_destroy(&(*f)->mutex) != 0) {
    return -1;
  }
  if (sem_destroy(&(*f)->full) != 0) {
    return -1;
  }
  if (sem_destroy(&(*f)->empty) != 0) {
    return -1;
  }
  if (munmap(*f, sizeof **f + (*f)->len * sizeof(pid_t)) != 0) {
    return -1;
  }
  *f = NULL;
  return 0;
}

int shm_fifo_unlink(const char *name) {
  return shm_unlink(name);
}
