#ifndef QKMER_H
#define QKMER_H

#include "sequence.h"
#include <stdint.h>
#include <stdbool.h>


Datum qkmer_in(PG_FUNCTION_ARGS);
Datum qkmer_out(PG_FUNCTION_ARGS);
Datum qkmer_cast_from_text(PG_FUNCTION_ARGS);
Datum qkmer_cast_to_text(PG_FUNCTION_ARGS);

char *qkmer_sequence_to_string(sequence *qkmer_seq);
char *qkmer_decode(uint8_t* qkmer_seq, size_t sequence_len);
sequence *qkmer_string_to_sequence(const char *qkmer_str);
uint8_t *qkmer_encode(const char *qkmer_str, const size_t sequence_len, size_t *data_bytes);
uint8_t qkmer_get_length(sequence* qkmer_seq);

bool qkmer_contains(sequence qkmer, sequence kmer);
bool match_Qsymbol (uint8_t symbol, uint8_t kmer_base);




#endif
