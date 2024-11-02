#include "kmer.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>


uint8_t kmer_get_length(KMER kmer) { return kmer.k; }

KMER kmer_string_to_KMER(const char *kmer_str) {
  KMER kmer;
  size_t str_len = strlen(kmer_str);
  if (str_len > 32) {
    printf("Invalid size for kmer (it can't be greater than 32)\n");
    kmer.k = -1;
    return kmer;
  }
  kmer.k = strlen(kmer_str);
  kmer.data = com_encode(kmer_str, kmer.k);
  return kmer;
}


char *kmer_KMER_to_string(KMER kmer) {
  return com_decode(kmer.data, kmer.k);
}


bool kmer_equals(KMER kmer1, KMER kmer2) {
  if (kmer1.k != kmer2.k) {
    return false;
  }

  for (size_t i = 0; i < kmer1.k; ++i) {
    if (kmer1.data[i] != kmer2.data[i]) {
      return false;
    }
  }

  return true;
}

void kmer_print_list_kmers(KMER *kmers, size_t num_kmers){
  printf("\n");
  for (size_t i = 0; i < num_kmers; ++i){
    printf("%s\n", kmer_KMER_to_string(kmers[i]));
  }
  printf("\n");
}
