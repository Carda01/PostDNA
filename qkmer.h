#ifndef QKMER_H
#define QKMER_H

#include "sequence.h"
#include <stdint.h>
#include <stdbool.h>



uint8_t qkmer_get_length(QKMER qkmer);
QKMER qkmer_string_to_QKMER(const char *qkmer_str);
uint8_t *qkmer_encode(const char *qkmer_str, uint8_t k);
char *qkmer_decode(uint8_t *qkmer_seq, uint8_t k);
char *qkmer_QKMER_to_string(QKMER qkmer);
bool qkmer_contains(QKMER sequence1, KMER sequence2);
bool match_Qsymbol (uint8_t symbol, uint8_t kmer_base);





#endif
