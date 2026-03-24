#include "../include/my_arena.h"
#include "../include/my_nn.h"
#include "../include/my_nn_loader.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int main() {
  const char *path = "dataset/traindata_set.bin";
  ui64 input_size = 700;
  ui64 actual_samples = 15731;

  FILE *f = fopen(path, "rb");
  if (!f) {
    fprintf(stderr, "yxsh: eval: %s: no such file or directory\n", path);
    return 1;
  }

  mem_arena_t arena = INIT_ARENA;
  arena_init(&arena, MiB(300), NULL);

  nn_t *nn = nn_load(&arena, "model/shell_trained_model.bin");

  mat_data_type *all_data = arena_push_arr(
      arena, mat_data_type, actual_samples * (input_size + 1), true, NULL);
  fread(all_data, sizeof(mat_data_type), actual_samples * (input_size + 1), f);
  fclose(f);

  double sum_sq_error = 0;
  matrix_t input_vec = mat_init(&arena, 1, input_size, NULL);

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  for (ui64 i = 0; i < actual_samples; i++) {
    mat_data_type *current_row = all_data + (i * (input_size + 1));
    memcpy(input_vec.data, current_row, input_size * sizeof(mat_data_type));
    mat_data_type true_label = current_row[input_size];

    matrix_t pred = nn_forward(&arena, nn, &input_vec);

    double diff = (double)MAT_AT(&pred, 0, 0) - (double)true_label;
    sum_sq_error += diff * diff;
  }

  clock_gettime(CLOCK_MONOTONIC, &end);

  double total_sec =
      (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
  double throughput = (double)actual_samples / total_sec;
  double rmse = sqrt(sum_sq_error / actual_samples);

  printf("Samples:   %lu\n", actual_samples);
  printf("RMSE:      %.4f\n", rmse);
  printf("Est. Err:  +/- %.1f cycles\n", rmse * 125.0);
  printf(
      "yxsh: pure inference latency: %.4f s | throughput: %.0f samples/sec\n",
      total_sec, throughput);
  printf("Avg latency per sample: %.2f us\n",
         (total_sec / actual_samples) * 1e6);

  arena_free(&arena);
  return 0;
}
