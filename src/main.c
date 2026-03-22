#include "../include/my_arena.h"
#include "../include/yxsh_core.h"
#include "yxsh_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  char errbuf[1024];
  mem_arena_t arena = INIT_ARENA;
  if (arena_init(&arena, MiB(256), errbuf) == -1) {
    fprintf(stderr, "yxsh: fatal error: failed to init memory arena: %s\n",
            errbuf);
    return 1;
  }

  command_status_t shell_status;
  shell_status.exit_status = 0;
  shell_status.command_counter = 0;
  shell_status.loaded_model = NULL;
  for (int i = 0; i < NUM_PIPE_MAX; i++) {
    shell_status.pipe_buffer[i] = -1;
  }

  char input_buffer[2048];
  printf("yxsh (AI-Powered Shell) - Type 'quit' or 'exit' to leave\n");

  while (1) {
    printf("yxsh> ");

    if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
      printf("\n");
      break;
    }

    input_buffer[strcspn(input_buffer, "\n")] = 0;

    if (strlen(input_buffer) == 0)
      continue;

    shell_status.exit_status = yxsh_run(&arena, input_buffer, &shell_status);
  }
  arena_free(&arena);
  return 0;
}
