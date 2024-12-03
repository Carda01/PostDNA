#ifndef QKMER_H
#define QKMER_H

#include "sequence.h"
#include <stdint.h>
#include <stdbool.h>


Datum qkmer_in(PG_FUNCTION_ARGS);
Datum qkmer_out(PG_FUNCTION_ARGS);
Datum qkmer_cast_from_text(PG_FUNCTION_ARGS);
Datum qkmer_cast_to_text(PG_FUNCTION_ARGS);
Datum qkmer_equals(PG_FUNCTION_ARGS);
Datum qkmer_starts_with(PG_FUNCTION_ARGS);
Datum qkmer_started_with(PG_FUNCTION_ARGS);
Datum qkmer_contains(PG_FUNCTION_ARGS);
Datum qkmer_is_contained(PG_FUNCTION_ARGS);

bool qkmer_contains_internal(sequence* qkmer, sequence* kmer);
bool match_Qsymbol (uint8_t symbol, uint8_t kmer_base);


#endif
