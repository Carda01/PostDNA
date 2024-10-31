#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <stdint.h>

#define BASE_MASK 0x3 // Binary: 11.

// Binary codes assigned to each DNA base.
enum {
  BASE_A = 0x0, // Binary: 00.
  BASE_C = 0x1, // Binary: 01.
  BASE_G = 0x2, // Binary: 10.
  BASE_T = 0x3, // Binary: 11.
};


typedef struct {
    size_t length;
    uint8_t *data;
} DNA;


typedef struct {
    uint8_t k;
    uint8_t *data;
} KMER;


void com_print_binary(const uint8_t value);
size_t com_get_number_of_bytes(size_t dna_len);

#endif
