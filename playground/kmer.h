#ifndef KMER_H
#define KMER_H

#include "common.h"
#include <stdint.h>

uint8_t kmer_get_length(KMER kmer);
KMER kmer_string_to_KMER(const char *kmer_str);
char *kmer_KMER_to_string(KMER);
uint8_t kmer_get_number_of_bytes(uint8_t kmer_len);
bool kmer_equals(KMER sequence1, KMER sequence2);

#endif
