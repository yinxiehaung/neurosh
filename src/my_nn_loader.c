#include "../include/my_nn_loader.h"

static ui32 swap_ui32(ui32 val) {
  return ((val << 24) & 0xFF000000) | ((val << 8) & 0x00FF0000) |
         ((val >> 8) & 0x0000FF00) | ((val >> 24) & 0x000000FF);
}

/*void load_mnist(const char *filename, matrix_t *m, ui32 num_classes) {
  FILE *f = fopen(filename, "rb");
  if (!f) {
    fprintf(stderr, "Could not open MNIST file: %s\n", filename);
    exit(1);
  }
  ui32 magic, count;
  fread(&magic, sizeof(ui32), 1, f);
  fread(&count, sizeof(ui32), 1, f);
  magic = swap_ui32(magic);
  count = swap_ui32(count);
  if (count != m->rows) {
    fprintf(stderr, "Size mismatch! File has %u items, Matrix has %lu rows.\n",
            count, m->rows);
    exit(1);
  }
  if (magic == 2051) {
    printf("[MNIST] Loading Images from %s...\n", filename);
    ui32 rows, cols;
    fread(&rows, sizeof(ui32), 1, f);
    fread(&cols, sizeof(ui32), 1, f);
    rows = swap_ui32(rows);
    cols = swap_ui32(cols);
    for (ui32 i = 0; i < count; i++) {
      for (ui32 j = 0; j < rows * cols; j++) {
        uint8_t pixel;
        fread(&pixel, 1, 1, f);
        MAT_AT(m, i, j) = (mat_data_type)pixel / 255.0f;
      }
    }
  } else if (magic == 2049) {
    printf("[MNIST] Loading Labels from %s...\n", filename);
    for (ui32 i = 0; i < count; i++) {
      uint8_t label;
      fread(&label, 1, 1, f);
      if (label >= num_classes) {
        fprintf(stderr, "Error: Label %d is out of bounds (max %d)\n", label,
                num_classes);
        exit(1);
      }
      for (ui32 k = 0; k < num_classes; k++) {
        MAT_AT(m, i, k) = 0.0f;
      }
      MAT_AT(m, i, label) = 1.0f;
    }
  } else {
    fprintf(stderr,
            "Unknown Magic Number: %u (Are you sure this is a MNIST file?)\n",
            magic);
    exit(1);
  }
  fclose(f);
  printf("Done.\n");
}
*/

void nn_save(nn_t *nn, const char *path) {
  FILE *f = fopen(path, "wb");
  if (!f) {
    fprintf(stderr, "Error: cannot open file to save model.\n");
    return;
  }

  nn_file_header_t header;
  header.magic = NN_MODEL_MAGIC;
  header.layer_count = nn->count;
  fwrite(&header, sizeof(header), 1, f);

  fwrite(nn->arch, sizeof(ui64), nn->arch_count, f);

  for (ui64 i = 0; i < nn->count; i++) {
    ui64 rows = nn->arch[i];
    ui64 cols = nn->arch[i + 1];
    fwrite(nn->ws[i].data, sizeof(mat_data_type), rows * cols, f);
    fwrite(nn->bs[i].data, sizeof(mat_data_type), 1 * cols, f);
  }

  fclose(f);
}

nn_t *nn_load(mem_arena_t *arena, const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;

  nn_file_header_t header;
  if (fread(&header, sizeof(header), 1, f) != 1 ||
      header.magic != NN_MODEL_MAGIC) {
    fclose(f);
    return NULL;
  }

  nn_t *nn = arena_push_type(*arena, nn_t, 1, NULL);
  nn->count = header.layer_count;
  nn->arch_count = header.layer_count + 1;

  nn->arch = arena_push_arr(*arena, ui64, nn->arch_count, true, NULL);
  fread(nn->arch, sizeof(ui64), nn->arch_count, f);

  nn->ws = arena_push_arr(*arena, matrix_t, nn->count, true, NULL);
  nn->bs = arena_push_arr(*arena, matrix_t, nn->count, true, NULL);
  nn->as = arena_push_arr(*arena, matrix_t, nn->arch_count, true, NULL);

  for (ui64 i = 0; i < nn->count; i++) {
    ui64 rows = nn->arch[i];
    ui64 cols = nn->arch[i + 1];
    nn->ws[i] = mat_init(arena, rows, cols, NULL);
    nn->bs[i] = mat_init(arena, 1, cols, NULL);
    fread(nn->ws[i].data, sizeof(mat_data_type), rows * cols, f);
    fread(nn->bs[i].data, sizeof(mat_data_type), 1 * cols, f);
  }

  fclose(f);
  return nn;
}
