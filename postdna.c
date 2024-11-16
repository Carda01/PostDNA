#include "sequence.h"
#include "dna.h"
#include "kmer.h"
#include "qkmer.h"
#include <stdint.h>
#include <stdio.h>

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(dna_in);
Datum dna_in(PG_FUNCTION_ARGS) {
  char *str = PG_GETARG_CSTRING(0);
  DNA* dna = dna_string_to_DNA(str);
  PG_RETURN_DNA_P(dna);
}

PG_FUNCTION_INFO_V1(dna_out);
Datum dna_out(PG_FUNCTION_ARGS) {
  DNA *dna = PG_GETARG_DNA_P(0);
  char *result = dna_DNA_to_string(dna);
  PG_FREE_IF_COPY(dna, 0);
  PG_RETURN_CSTRING(result);
}

PG_FUNCTION_INFO_V1(dna_cast_from_text);
Datum dna_cast_from_text(PG_FUNCTION_ARGS) {
  text *txt = PG_GETARG_TEXT_P(0);
  char *str = text_to_cstring(txt);
  PG_RETURN_DNA_P(dna_string_to_DNA(&str));
}

PG_FUNCTION_INFO_V1(dna_cast_to_text);
Datum dna_cast_to_text(PG_FUNCTION_ARGS) {
  DNA *dna = PG_GETARG_DNA_P(0);
  text *out = cstring_to_text(dna_DNA_to_string(dna));
  PG_FREE_IF_COPY(dna, 0);
  PG_RETURN_TEXT_P(out);
}
