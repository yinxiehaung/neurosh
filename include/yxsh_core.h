#ifndef YXSH_H
#define YXSH_H
#include "my_arena.h"
#include "my_nn.h"
#include "my_string.h"
#define NUM_PIPE_MAX 1024

typedef struct command_status_s command_status_t;

struct command_status_s {
  int exit_status;
  int pipe_buffer[NUM_PIPE_MAX];
  ui64 command_counter;
  nn_t *loaded_model;
};

int yxsh_run(mem_arena_t *arena, const char *command_char,
             command_status_t *status);
#endif
