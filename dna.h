#ifndef DNA_H
#define DNA_H

#include "common.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>


size_t dna_get_length(DNA dna);
DNA dna_string_to_DNA(const char *dna_str);
char *dna_DNA_to_string(DNA);
bool dna_equals(DNA sequence1, DNA sequence2);

KMER* dna_generate_kmers(DNA dna, uint8_t k);

#endif
