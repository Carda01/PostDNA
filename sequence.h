#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <stdint.h>
#include "postgres.h"
#include "fmgr.h"

#define BASE_MASK 0x3 // Binary: 11.

#define QUERY_MASK 0xF // Binary: 1111.


// Binary codes assigned to each DNA base.
enum {
  BASE_A = 0x0, // Binary: 00.
  BASE_C = 0x1, // Binary: 01.
  BASE_G = 0x2, // Binary: 10.
  BASE_T = 0x3, // Binary: 11.

  BASE_R = 0x4, // Binary: 0100, A or G 
  BASE_Y = 0x5, // Binary: 0101, C or T 
  BASE_S = 0x6, // Binary: 0110, G or C 
  BASE_W = 0x7, // Binary: 0111, A or T 
  BASE_K = 0x8, // Binary: 1000, G or T 
  BASE_M = 0x9, // Binary: 1001, A or C 

  BASE_B = 0xA, // Binary: 1010, C, G, or T 
  BASE_D = 0xB, // Binary: 1011, A, G, or T 
  BASE_H = 0xC, // Binary: 1100, A, C, or T
  BASE_V = 0xD, // Binary: 1101, A, C, or G

  BASE_N = 0xE, // Binary: 1110, A, C, G, or T (any base)
};

typedef struct {
    int32 struct_size;
    uint8_t overflow;
    uint8_t data[FLEXIBLE_ARRAY_MEMBER];
} sequence;

typedef struct {
    int32 struct_size;
    uint8_t overflow;
    uint8_t data[FLEXIBLE_ARRAY_MEMBER];
} DNA;


typedef struct {
    uint8_t k;
    uint8_t *data;
} KMER;

typedef struct {
    uint8_t k;
    uint8_t *data;
} QKMER;


void seq_print_binary(const uint8_t value);
size_t seq_get_number_of_bytes(size_t dna_len);
uint8_t* seq_encode(const char *sequence, const size_t sequence_len, size_t *data_bytes);
char* seq_decode(uint8_t* data, size_t sequence_len);

size_t seq_get_length(DNA* dna);
size_t seq_get_num_generable_kmers(size_t dna_len, uint8_t k);

#endif
