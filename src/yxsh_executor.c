#include "yxsh_internal.h"

typedef int (*buildin_func_t)(exe_ctx_t *ctx, shell_AST_t *ast);

static int buildin_exit(exe_ctx_t *ctx, shell_AST_t *ast) { exit(0); }
static int buildin_setenv(exe_ctx_t *ctx, shell_AST_t *ast) {
  if (ast->argc >= 3) {
    setenv(ast->argv[1].str, ast->argv[2].str, 1);
  }
  return 0;
}

static int buildin_printenv(exe_ctx_t *ctx, shell_AST_t *ast) {
  if (ast->argc == 1) {
    char **env = environ;
    for (ui64 i = 0; env[i] != NULL; i++) {
      printf("%s\n", env[i]);
    }
  } else if (ast->argc >= 2) {
    mem_tmp_arena_t tmp = arena_begin_tmp(ctx->arena);
    char *key = str_to_cstr(ctx->arena, &ast->argv[1]);
    if (key == NULL)
      return 0;
    char *s = getenv(key);
    if (s)
      printf("%s\n", s);
    arena_end_tmp(&tmp);
  }
  return 0;
}

static int buildin_cd(exe_ctx_t *ctx, shell_AST_t *ast) {
  if (ast->argc >= 2) {
    mem_tmp_arena_t tmp = arena_begin_tmp(ctx->arena);
    char *path = str_to_cstr(ctx->arena, &ast->argv[1]);
    chdir(path);
    arena_end_tmp(&tmp);
  }
  return 0;
}

static ui64 get_sample_count(const char *path, ui64 row_size_bytes) {
  struct stat st;
  if (stat(path, &st) == 0)
    return st.st_size / row_size_bytes;
  return 0;
}

/* --- 1. Train Command --- */
static int buildin_nn_train(exe_ctx_t *ctx, shell_AST_t *ast) {
  // 1. 更新參數數量檢查 (新增了 in_size 和 out_size)
  if (ast->argc < 8) {
    fprintf(stderr, "yxsh: train: missing arguments\n");
    fprintf(stderr, "Usage: train <data.bin> <in_size> <out_size> <rate> "
                    "<epochs> <batch> <h1> [h2...]\n");
    return 0;
  }

  const char *path = str_to_cstr(ctx->arena, &ast->argv[1]);

  // 2. 動態讀取輸入與輸出維度
  ui64 input_size = (ui64)atoll(str_to_cstr(ctx->arena, &ast->argv[2]));
  ui64 output_size = (ui64)atoll(str_to_cstr(ctx->arena, &ast->argv[3]));

  ui64 row_size_bytes = (input_size + output_size) * sizeof(mat_data_type);
  ui64 total_samples = get_sample_count(path, row_size_bytes);

  if (total_samples == 0) {
    fprintf(stderr, "yxsh: train: %s: no such file or invalid format\n", path);
    return 1;
  }

  // 3. 參數往後位移，讀取超參數
  mat_data_type rate =
      (mat_data_type)atof(str_to_cstr(ctx->arena, &ast->argv[4]));
  ui64 epochs = (ui64)atoll(str_to_cstr(ctx->arena, &ast->argv[5]));
  ui64 batch_size = (ui64)atoll(str_to_cstr(ctx->arena, &ast->argv[6]));

  ui64 hidden_count = ast->argc - 7;
  ui64 arch_count = hidden_count + 2;
  ui64 *arch = arena_push_arr(*ctx->arena, ui64, arch_count, true, NULL);

  // 4. 動態設定網路首尾架構
  arch[0] = input_size;
  for (ui64 i = 0; i < hidden_count; i++) {
    arch[i + 1] = (ui64)atoll(str_to_cstr(ctx->arena, &ast->argv[i + 7]));
  }
  arch[arch_count - 1] = output_size;

  matrix_t train_x = mat_init(ctx->arena, total_samples, input_size, NULL);
  matrix_t train_y = mat_init(ctx->arena, total_samples, output_size, NULL);

  FILE *f = fopen(path, "rb");
  if (!f) {
    fprintf(stderr, "yxsh: train: %s: permission denied or file not found\n",
            path);
    return 0;
  }

  mat_data_type *temp_row = arena_push_arr(
      *ctx->arena, mat_data_type, input_size + output_size, true, NULL);

  for (ui64 i = 0; i < total_samples; i++) {
    if (fread(temp_row, sizeof(mat_data_type), input_size + output_size, f) !=
        input_size + output_size)
      break;

    // 5. 複製特徵 (X)
    memcpy(&MAT_AT(&train_x, i, 0), temp_row,
           input_size * sizeof(mat_data_type));

    // 6. 動態複製標籤 (Y)，支援多輸出神經網路！
    // temp_row + input_size 代表指標往前跳過特徵的記憶體區塊
    memcpy(&MAT_AT(&train_y, i, 0), temp_row + input_size,
           output_size * sizeof(mat_data_type));
  }
  fclose(f);

  nn_t *nn = arena_push_type(*ctx->arena, nn_t, 1, NULL);
  *nn = nn_init(ctx->arena, arch, arch_count, NULL);
  nn_cfg_t cfg = nn_cfg_init(rate, 0.001f, batch_size, epochs);

  printf("yxsh: training started [%lu samples, in: %lu, out: %lu]\n",
         total_samples, input_size, output_size);

  nn_train(ctx->arena, nn, &train_x, &train_y, &cfg);

  ctx->status->loaded_model = nn;
  nn_save(nn, "shell_trained_model.bin");
  printf("yxsh: model saved to 'shell_trained_model.bin'\n");

  return 0;
}

/* --- 2. Eval Command --- */
static int buildin_nn_eval(exe_ctx_t *ctx, shell_AST_t *ast) {
  if (!ctx->status->loaded_model) {
    fprintf(stderr, "yxsh: eval: no model loaded\n");
    return 0;
  }
  if (ast->argc < 2) {
    fprintf(stderr, "Usage: eval <test_data.bin>\n");
    return 0;
  }

  const char *path = str_to_cstr(ctx->arena, &ast->argv[1]);
  ui64 input_size = ctx->status->loaded_model->arch[0];
  ui64 row_size_bytes = (input_size + 1) * sizeof(mat_data_type);
  ui64 total_samples = get_sample_count(path, row_size_bytes);

  FILE *f = fopen(path, "rb");
  if (!f) {
    fprintf(stderr, "yxsh: eval: %s: no such file or directory\n", path);
    return 0;
  }

  double sum_sq_error = 0;
  mem_tmp_arena_t tmp = arena_begin_tmp(ctx->arena);
  mat_data_type *row_ptr =
      arena_push_arr(*tmp.arena, mat_data_type, input_size + 1, true, NULL);
  matrix_t input_vec = mat_init(tmp.arena, 1, input_size, NULL);

  for (ui64 i = 0; i < total_samples; i++) {
    if (fread(row_ptr, sizeof(mat_data_type), input_size + 1, f) !=
        input_size + 1)
      break;
    memcpy(input_vec.data, row_ptr, input_size * sizeof(mat_data_type));
    mat_data_type true_label = row_ptr[input_size];

    matrix_t pred =
        nn_forward(tmp.arena, ctx->status->loaded_model, &input_vec);
    double diff = (double)MAT_AT(&pred, 0, 0) - (double)true_label;
    sum_sq_error += diff * diff;
  }
  fclose(f);

  double rmse = sqrt(sum_sq_error / total_samples);
  printf("Samples:  %lu\n", total_samples);
  printf("RMSE:     %.4f\n", rmse);
  printf("Est. Err: +/- %.1f cycles\n", rmse * 125.0);

  arena_end_tmp(&tmp);
  return 0;
}

/* --- 3. Visualize Command --- */
static int buildin_nn_visualize(exe_ctx_t *ctx, shell_AST_t *ast) {
  if (!ctx->status->loaded_model) {
    fprintf(stderr, "yxsh: visualize: no model loaded\n");
    return 0;
  }
  if (ast->argc < 2) {
    fprintf(stderr, "Usage: visualize <test_data.bin>\n");
    return 0;
  }

  const char *path = str_to_cstr(ctx->arena, &ast->argv[1]);
  FILE *f = fopen(path, "rb");
  if (!f) {
    fprintf(stderr, "yxsh: visualize: %s: file not found\n", path);
    return 0;
  }

  ui64 input_size = ctx->status->loaded_model->arch[0];
  fseek(f, 0, SEEK_END);
  ui64 total_points = ftell(f) / (input_size * sizeof(mat_data_type));
  rewind(f);

  // Standard output format suitable for piping or logging
  printf("Health | Timeline -----> Failure\n");
  printf("-------+----------------------------------------\n");

  mem_tmp_arena_t tmp = arena_begin_tmp(ctx->arena);
  matrix_t input_vec = mat_init(tmp.arena, 1, input_size, NULL);

  // Scale dynamically to keep terminal output clean (~40 rows max)
  int step = (total_points > 40) ? (total_points / 40) : 1;

  for (ui64 i = 0; i < total_points; i++) {
    fread(input_vec.data, sizeof(mat_data_type), input_size, f);
    if (i % step == 0) {
      matrix_t pred =
          nn_forward(tmp.arena, ctx->status->loaded_model, &input_vec);
      float health = 1.0f - MAT_AT(&pred, 0, 0);

      printf("  %3.1f  | ", health);
      int bars = (int)(health * 20); // Scale up to 20 ASCII characters
      for (int j = 0; j < bars; j++)
        printf("#");
      printf("\n");
    }
  }

  fclose(f);
  arena_end_tmp(&tmp);
  return 0;
}

static int buildin_nn_load(exe_ctx_t *ctx, shell_AST_t *ast) {
  if (ast->argc < 2) {
    fprintf(stderr, "Usage: nn_load <model.bin>\n");
    return 0;
  }

  const char *path = str_to_cstr(ctx->arena, &ast->argv[1]);

  nn_t *nn = nn_load(ctx->arena, path);
  if (!nn) {
    fprintf(stderr,
            "yxsh: nn_load: %s: failed to load model (file missing or invalid "
            "format)\n",
            path);
    return 0;
  }

  ctx->status->loaded_model = nn;

  printf("yxsh: model loaded [%lu layers, input: %lu, output: %lu]\n",
         nn->count, nn->arch[0], nn->arch[nn->count]);

  return 0;
}

static int buildin_nn_predict(exe_ctx_t *ctx, shell_AST_t *ast) {
  if (!ctx->status->loaded_model) {
    fprintf(stderr, "yxsh: predict: no model loaded\n");
    return 1;
  }
  if (ast->argc < 2) {
    fprintf(stderr, "Usage: predict <test_data.bin>\n");
    return 1;
  }

  const char *path = str_to_cstr(ctx->arena, &ast->argv[1]);
  FILE *f = fopen(path, "rb");
  if (!f) {
    fprintf(stderr, "yxsh: predict: %s: file not found\n", path);
    return 1;
  }

  ui64 input_size = ctx->status->loaded_model->arch[0];
  mem_tmp_arena_t tmp = arena_begin_tmp(ctx->arena);
  matrix_t input_vec = mat_init(tmp.arena, 1, input_size, NULL);

  if (fread(input_vec.data, sizeof(mat_data_type), input_size, f) !=
      input_size) {
    fprintf(stderr, "yxsh: predict: %s: read error or EOF\n", path);
    fclose(f);
    arena_end_tmp(&tmp);
    return 1;
  }
  fclose(f);

  matrix_t pred = nn_forward(tmp.arena, ctx->status->loaded_model, &input_vec);
  float prob = MAT_AT(&pred, 0, 0);
  float rul = 125.0f * (1.0f - prob);
  printf("PREDICT_RUL=%.1f\n", rul);
  printf("PREDICT_PROB=%.4f\n", prob);

  arena_end_tmp(&tmp);
  return 0;
}

struct {
  const char *name;
  buildin_func_t func;
} buildin[] = {{"cd", buildin_cd},
               {"setenv", buildin_setenv},
               {"printenv", buildin_printenv},
               {"exit", buildin_exit},
               {"quit", buildin_exit},
               {"train", buildin_nn_train},
               {"eval", buildin_nn_eval},
               {"plot", buildin_nn_visualize},
               {"nnload", buildin_nn_load},
               {"infer", buildin_nn_predict},
               {"score", buildin_nn_eval},
               {NULL, NULL}};

static int execute_ast(exe_ctx_t *ctx, shell_AST_t *ast);

static int str_execvp(mem_arena_t *arena, string_t *argv, ui64 argc) {
  char **c_argv = arena_push_arr(*arena, char *, argc + 1, 1, NULL);
  for (ui64 i = 0; i < argc; i++) {
    char *c_str = str_to_cstr(arena, &argv[i]);
    c_argv[i] = c_str;
  }
  c_argv[argc] = NULL;
  return execvp(c_argv[0], c_argv);
}

static void proc_status(exe_ctx_t *ctx, int status) {
  if (WIFEXITED(status)) {
    ctx->status->exit_status = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    ctx->status->exit_status = 128 + WTERMSIG(status);
  } else {
    ctx->status->exit_status = -1;
  }
}

static void __exe_command(exe_ctx_t *ctx, shell_AST_t *ast) {
  if (ast->file_in.str != NULL) {
    char *in_file = str_to_cstr(ctx->arena, &ast->file_in);
    int fd = open(in_file, O_RDONLY);
    if (fd == -1) {
      perror("yxsh:input file error\n");
      exit(1);
    }
    dup2(fd, STDIN_FILENO);
    close(fd);
  }
  if (ast->file_out.str != NULL) {
    char *out_file = str_to_cstr(ctx->arena, &ast->file_out);
    int flag = O_CREAT | O_WRONLY;
    if (ast->is_append) {
      flag |= O_APPEND;
    } else {
      flag |= O_TRUNC;
    }
    int fd = open(out_file, flag, 0644);
    if (fd == -1) {
      perror("yxsh: output file error\n");
      exit(1);
    }
    dup2(fd, STDOUT_FILENO);
    close(fd);
  }
  str_execvp(ctx->arena, ast->argv, ast->argc);
  fprintf(stderr, "yxsh: command not found\n");
  exit(127);
}

static int check_and_run_internal_command(exe_ctx_t *ctx, shell_AST_t *ast) {
  for (int i = 0; buildin[i].name != NULL; i++) {
    if (str_cmp_char(&ast->argv[0], buildin[i].name)) {
      ctx->status->exit_status = buildin[i].func(ctx, ast);
      return ctx->status->exit_status;
    }
  }
  return -1;
}

static int exe_command(exe_ctx_t *ctx, shell_AST_t *ast) {
  for (ui64 i = 0; i < ast->argc; i++) {
    ast->argv[i] = shell_expand(ctx, &ast->argv[i]);
  }
  if (check_and_run_internal_command(ctx, ast) == 0) {
    return 0;
  }
  pid_t child_pid = fork();
  if (child_pid == 0) {
    __exe_command(ctx, ast);
  } else if (child_pid > 0) {
    int status;
    waitpid(child_pid, &status, 0);
    proc_status(ctx, status);
    return ctx->status->exit_status;
  }
  perror("fork error");
  return -1;
}

static int exe_num_pipe(exe_ctx_t *ctx, shell_AST_t *ast) { return 0; }

static int exe_pipe(exe_ctx_t *ctx, shell_AST_t *ast) {
  int pipefd[2];
  pipe(pipefd);

  pid_t left_pid = fork();
  if (left_pid == -1) {
    perror("fork error");
    return -1;
  } else if (left_pid == 0) {
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[0]);
    close(pipefd[1]);
    exit(execute_ast(ctx, ast->left));
  }
  pid_t right_pid = fork();
  if (right_pid == -1) {
    perror("fork error");
    return -1;
  } else if (right_pid == 0) {
    dup2(pipefd[0], STDIN_FILENO);
    close(pipefd[0]);
    close(pipefd[1]);
    exit(execute_ast(ctx, ast->right));
  }

  close(pipefd[0]);
  close(pipefd[1]);

  int left_status, right_status;
  waitpid(left_pid, &left_status, 0);
  waitpid(right_pid, &right_status, 0);
  proc_status(ctx, right_status);
  return ctx->status->exit_status;
}

static int exe_and_or(exe_ctx_t *ctx, shell_AST_t *ast) {
  int left_status = execute_ast(ctx, ast->left);
  if (ast->state == AST_NODE_DOUBLE_AND && left_status != 0) {
    return left_status;
  }
  if (ast->state == AST_NODE_OR && left_status == 0) {
    return left_status;
  }
  if (ast->right == NULL) {
    return left_status;
  }
  return execute_ast(ctx, ast->right);
}

static int exe_list(exe_ctx_t *ctx, shell_AST_t *ast) {
  if (ast->state == AST_NODE_SEQUENCE) {
    execute_ast(ctx, ast->left);
    if (ast->right != NULL) {
      return execute_ast(ctx, ast->right);
    }
    return ctx->status->exit_status;
  } else if (ast->state == AST_NODE_AND) {
    pid_t bg_pid = fork();
    if (bg_pid == 0) {
      if (ast->left->file_in.str == NULL) {
        ast->left->file_in = str_new_static_in(*ctx->arena, "/dev/null");
      }
      exit(execute_ast(ctx, ast->left));
    } else if (bg_pid > 0) {
      printf("[1] %d\n", bg_pid);
      ctx->status->exit_status = 0;
      return 0;
    }
  }
  return ctx->status->exit_status;
}

static int execute_ast(exe_ctx_t *ctx, shell_AST_t *root) {
  switch (root->state) {
  case AST_NODE_COMMAND:
    return exe_command(ctx, root);
  case AST_NODE_PIPE:
    return exe_pipe(ctx, root);
  case AST_NODE_NUMBER_PIPE:
    return exe_num_pipe(ctx, root);
  case AST_NODE_OR:
  case AST_NODE_DOUBLE_AND:
    return exe_and_or(ctx, root);
  case AST_NODE_SEQUENCE:
  case AST_NODE_AND:
    return exe_list(ctx, root);
  default:
    break;
  }
  return ctx->status->exit_status;
}

int shell_executor(mem_arena_t *arena, shell_AST_t *root,
                   command_status_t *prev_status) {
  exe_ctx_t ctx = {.arena = arena, .status = prev_status};
  if (root == NULL)
    return -1;
  return execute_ast(&ctx, root);
}
