#ifndef KMER_H
#define KMER_H

#include "sequence.h"

Datum kmer_in(PG_FUNCTION_ARGS);
Datum kmer_out(PG_FUNCTION_ARGS);
Datum kmer_cast_from_text(PG_FUNCTION_ARGS);
Datum kmer_cast_to_text(PG_FUNCTION_ARGS);
Datum kmer_equals(PG_FUNCTION_ARGS);
Datum kmer_nequals(PG_FUNCTION_ARGS);
Datum kmer_starts_with(PG_FUNCTION_ARGS);
Datum kmer_started_with(PG_FUNCTION_ARGS);
Datum kmer_canonicalize(PG_FUNCTION_ARGS);
Datum kmer_hash(PG_FUNCTION_ARGS);
Datum kmer_typmod_in(PG_FUNCTION_ARGS);
Datum kmer_typmod_out(PG_FUNCTION_ARGS);
Datum kmer_typmod_cast(PG_FUNCTION_ARGS);


uint8_t kmer_get_base_at_index(const uint8_t* data, int index);
void kmer_set_base_at_index(uint8_t* data, int index, uint8_t nodeBase);
sequence *kmer_internal_canonicalize(sequence *kmer);

Datum kmer_create_subseq(const uint8_t *data, int begin, int datalen);
void kmer_fill_copy(uint8_t* target, uint8_t* source, int target_start, int length);

#endif
