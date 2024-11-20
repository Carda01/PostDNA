#ifndef COMMON_H
#define COMMON_H

#include "postgres.h"
#include <stdlib.h>
#include <stdint.h>
#include "fmgr.h"
#include "libpq/pqformat.h"
#include "utils/fmgrprotos.h"

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

enum {
  DNA = 0,
  KMER = 1,
  QKMER = 2,
};

#define KMER_MAX_SIZE 32


#define DatumGetSEQP(X)  ((sequence *) DatumGetPointer(X))
#define SEQPGetDatum(X)  PointerGetDatum(X)
#define PG_GETARG_SEQ_P(n) DatumGetSEQP(PG_GETARG_DATUM(n))
#define PG_RETURN_SEQ_P(x) return SEQPGetDatum(x)

typedef struct {
    int32 struct_size;
    uint8_t overflow;
    uint8_t data[FLEXIBLE_ARRAY_MEMBER];
} sequence;


Datum generate_kmers(PG_FUNCTION_ARGS); 

char* seq_get_byte_binary_representation(const uint8_t value);

sequence* seq_string_to_sequence(const char *seq_str, int type);
char *seq_sequence_to_string(sequence *seq, int type);

uint8_t *seq_encode(const char *seq_str, const size_t sequence_len, size_t *data_bytes, int type);
char* seq_decode(uint8_t* data, size_t sequence_len, int type);

inline size_t seq_get_number_of_bytes(size_t seq_len, int type);
inline size_t seq_get_length(sequence* seq, int type);
inline uint8_t seq_get_overflow(size_t seq_length, int type);

size_t seq_get_num_generable_kmers(size_t seq_len, uint8_t k);

bool seq_equals(sequence* seq1, sequence* seq2, int type);

sequence* seq_generate_kmers(sequence* seq, uint8_t k);

inline char* seq_get_string_type(int type);
inline int seq_bases_per_byte(int type);

#endif
