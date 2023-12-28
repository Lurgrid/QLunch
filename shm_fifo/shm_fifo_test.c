// Programme présantant un exemple d'utilisation du module shm_fifo

#include <stdlib.h>
#include <stdio.h>
#include "shm_fifo.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

#define NAME "shm_custom"
#define LEN 2048

#define NB 16

int main(void) {
  int r = EXIT_SUCCESS;
  fifo_t *f = shm_fifo_init(NAME, LEN);
  if (f == NULL) {
    perror("shm_fifo_init");
    return EXIT_FAILURE;
  }
  switch (fork()) {
    case -1:
      perror("fork");
      goto error;
    case 0:
      pid_t p = shm_fifo_dequeue(f);
      while (p != (pid_t) -1 && p != (pid_t) 0) {
        printf("pidd: %d\n", p);
        p = shm_fifo_dequeue(f);
      }
      if (p == (pid_t) -1) {
        perror("dequeue");
        return EXIT_FAILURE;
      }
      return EXIT_SUCCESS;
    default:
      for (size_t i = 0; i < NB; ++i) {
        if (shm_fifo_enqueue(f, getpid()) != 0) {
          perror("shm_fifo_enqueue");
          goto error;
        }
      }
      if (shm_fifo_enqueue(f, 0) != 0) {
        perror("shm_fifo_enqueue");
        goto error;
      }
  }
  if (wait(NULL) == (pid_t) -1) {
    perror("wait");
    goto error;
  }
  goto dispose;
error:
  r = EXIT_FAILURE;
dispose:
  if (shm_fifo_dispose(&f) != 0) {
    perror("shm_fifo_dispose");
    r = EXIT_FAILURE;
  }
  if (shm_fifo_unlink(NAME) != 0) {
    perror("shm_fifo_unlink");
    r = EXIT_FAILURE;
  }
  return r;
}
