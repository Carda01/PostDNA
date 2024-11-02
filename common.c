#include "common.h"
#include <stdio.h>

size_t com_get_number_of_bytes(size_t dna_len) {
  return (dna_len / 4) + (dna_len % 4 != 0);
}

void com_print_binary(const uint8_t value) {
  for (int i = 7; i >= 0; i--) {
    printf("%d", (value >> i) & 1);
  }
  printf("\n");
}
