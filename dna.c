#include "dna.h"
#include "common.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

size_t dna_get_length(DNA dna) { return dna.length; }

DNA dna_string_to_DNA(const char *dna_str) {
  DNA dna;
  dna.length = strlen(dna_str);
  dna.data = com_encode(dna_str, dna.length);
  return dna;
}

char *dna_DNA_to_string(DNA dna) { return com_decode(dna.data, dna.length); }

bool dna_equals(DNA dna1, DNA dna2) {
  if (dna1.length != dna2.length) {
    return false;
  }

  for (size_t i = 0; i < dna1.length; ++i) {
    if (dna1.data[i] != dna2.data[i]) {
      return false;
    }
  }

  return true;
}

KMER *dna_generate_kmers(DNA dna, uint8_t k) {
  if (k > 32) {
    printf("Invalid k size (>32)");
    return NULL;
  }

  const size_t num_kmers = com_get_num_generable_kmers(dna.length, k);
  const uint8_t data_bytes = com_get_number_of_bytes(k);
  KMER *kmers = malloc(sizeof(KMER) * num_kmers);
  for (size_t i = 0; i < num_kmers; ++i) {
    KMER kmer;
    kmer.k = k;
    kmer.data = malloc(sizeof(uint8_t) * data_bytes);
    size_t init_byte = (2 * i) / 8;
    uint8_t init_pos = (2 * i) % 8;
    uint8_t step = 2 * k - 1;
    for (size_t c = 0; c < data_bytes; ++c) {
      kmer.data[c] = dna.data[c + init_byte];
      kmer.data[c] <<= init_pos;
      if ((init_pos + step) / 8 != c){
        kmer.data[c] |= dna.data[c + init_byte + 1] >> (8 - init_pos);
      }
    }
    kmers[i] = kmer;
  }

  return kmers;
}
