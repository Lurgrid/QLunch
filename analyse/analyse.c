#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "analyse.h"

char **analyse_arg(const char arg[]) {
  const char *parg = arg;
  while (*parg == ' ') {
    ++parg;
  }
  size_t len = strlen(parg) + 1;
  char *s = malloc(len);
  if (s == NULL) {
    return NULL;
  }
  strcpy(s, parg);
  size_t argc = 0;
  bool space_before = true;
  for (size_t i = 0; i < len; ++i) {
    if (s[i] != '\0' && s[i] != ' ' && space_before) {
      ++argc;
    }
    space_before = s[i] == ' ';
    if (space_before) {
      s[i] = '\0';
    }
  }
  char **argv = malloc(sizeof *argv * (argc + 1));
  if (argv == NULL) {
    free(s);
    return NULL;
  }
  argv[argc] = NULL;
  size_t p = 0;
  bool in_arg = false;
  for (size_t i = 0; i < len; ++i) {
    if (s[i] != '\0' && !in_arg) {
      argv[p] = &s[i];
      ++p;
    }
    in_arg = s[i] != '\0';
  }
  return argv;
}

void dispose_arg(char *argv[]) {
  free(argv[0]);
  free(argv);
}
