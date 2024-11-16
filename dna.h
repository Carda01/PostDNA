#ifndef DNA_H
#define DNA_H

#include "sequence.h"

Datum dna_in(PG_FUNCTION_ARGS);
Datum dna_out(PG_FUNCTION_ARGS);
Datum dna_cast_from_text(PG_FUNCTION_ARGS);
Datum dna_cast_to_text(PG_FUNCTION_ARGS);

#endif
