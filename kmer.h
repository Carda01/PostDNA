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
Datum kmer_hash(PG_FUNCTION_ARGS);
#endif
