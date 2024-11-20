#include "kmer.h"
#include "sequence.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

PG_FUNCTION_INFO_V1(kmer_in);
Datum kmer_in(PG_FUNCTION_ARGS) {
    char *str = PG_GETARG_CSTRING(0);
    sequence* seq = seq_string_to_sequence(str, KMER);
    PG_RETURN_SEQ_P(seq);
}

PG_FUNCTION_INFO_V1(kmer_out);
Datum kmer_out(PG_FUNCTION_ARGS) {
    sequence *seq = PG_GETARG_SEQ_P(0);
    char *result = seq_sequence_to_string(seq, KMER);
    PG_FREE_IF_COPY(seq, 0);
    PG_RETURN_CSTRING(result);
}

PG_FUNCTION_INFO_V1(kmer_cast_from_text);
Datum kmer_cast_from_text(PG_FUNCTION_ARGS) {
    text *txt = PG_GETARG_TEXT_P(0);
    char *str = DatumGetCString(DirectFunctionCall1(textout, PointerGetDatum(txt)));
    PG_RETURN_SEQ_P(seq_string_to_sequence(&str, KMER));
}

PG_FUNCTION_INFO_V1(kmer_cast_to_text);
Datum kmer_cast_to_text(PG_FUNCTION_ARGS) {
    sequence *seq = PG_GETARG_SEQ_P(0);
    text *out = (text *)DirectFunctionCall1(textin, PointerGetDatum(seq_sequence_to_string(seq, KMER)));
    PG_FREE_IF_COPY(seq, 0);
    PG_RETURN_TEXT_P(out);
}


PG_FUNCTION_INFO_V1(kmer_equals);
Datum kmer_equals(PG_FUNCTION_ARGS) {
  sequence *kmer1 = PG_GETARG_SEQ_P(0);
  sequence *kmer2 = PG_GETARG_SEQ_P(1);
  bool result = seq_equals(kmer1, kmer2, KMER);
  PG_FREE_IF_COPY(kmer1, 0);
  PG_FREE_IF_COPY(kmer2, 1);
  PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(kmer_nequals);
Datum kmer_nequals(PG_FUNCTION_ARGS) {
  sequence *kmer1 = PG_GETARG_SEQ_P(0);
  sequence *kmer2 = PG_GETARG_SEQ_P(1);
  bool result = !seq_equals(kmer1, kmer2, KMER);
  PG_FREE_IF_COPY(kmer1, 0);
  PG_FREE_IF_COPY(kmer2, 1);
  PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(kmer_hash);
Datum kmer_hash(PG_FUNCTION_ARGS) {
    sequence *seq = PG_GETARG_SEQ_P(0);
    int32 out = seq_hash(seq, KMER); 
    PG_FREE_IF_COPY(seq, 0);
    PG_RETURN_INT32(out);
}
