#ifndef KMER_H
#define KMER_H

#include "sequence.h"

Datum kmer_in(PG_FUNCTION_ARGS); 
Datum kmer_out(PG_FUNCTION_ARGS); 
Datum kmer_cast_from_text(PG_FUNCTION_ARGS); 
Datum kmer_cast_to_text(PG_FUNCTION_ARGS); 

#endif
