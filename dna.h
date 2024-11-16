#ifndef DNA_H
#define DNA_H

#include "common.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define DatumGetDNAP(X)  ((DNA *) DatumGetPointer(X))
#define DNAPGetDatum(X)  PointerGetDatum(X)
#define PG_GETARG_DNA_P(n) DatumGetDNAP(PG_GETARG_DATUM(n))
#define PG_RETURN_DNA_P(x) return DNAPGetDatum(x)


size_t dna_get_length(DNA* dna);
DNA *dna_string_to_DNA(const char *dna_str);
char *dna_DNA_to_string(DNA* dna);
bool dna_equals(DNA* sequence1, DNA* sequence2);

KMER* dna_generate_kmers(DNA* dna, uint8_t k);

/*****************************************************************************/

//Datum dna_DNA_in(PG_FUNCTION_ARGS);
//Datum dna_DNA_out(PG_FUNCTION_ARGS);
//Datum dna_DNA_cast_from_text(PG_FUNCTION_ARGS);
//Datum dna_DNA_cast_to_text(PG_FUNCTION_ARGS);

#endif
