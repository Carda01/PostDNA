#include "kmer.h"
#include "sequence.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

PG_FUNCTION_INFO_V1(kmer_in);
Datum kmer_in(PG_FUNCTION_ARGS) {
    globalQkmerFlag = 1;
    char *str = PG_GETARG_CSTRING(0);
    seq_kmer_check_length(str);
    sequence* seq = seq_string_to_sequence(str);
    PG_RETURN_SEQ_P(seq);
}

PG_FUNCTION_INFO_V1(kmer_out);
Datum kmer_out(PG_FUNCTION_ARGS) {
    globalQkmerFlag = 1;
    sequence *seq = PG_GETARG_SEQ_P(0);
    char *result = seq_sequence_to_string(seq);
    PG_FREE_IF_COPY(seq, 0);
    PG_RETURN_CSTRING(result);
}

PG_FUNCTION_INFO_V1(kmer_cast_from_text);
Datum kmer_cast_from_text(PG_FUNCTION_ARGS) {
    globalQkmerFlag = 1;
    text *txt = PG_GETARG_TEXT_P(0);
    char *str = DatumGetCString(DirectFunctionCall1(textout, PointerGetDatum(txt)));
    seq_kmer_check_length(str);
    PG_RETURN_SEQ_P(seq_string_to_sequence(&str));
}

PG_FUNCTION_INFO_V1(kmer_cast_to_text);
Datum kmer_cast_to_text(PG_FUNCTION_ARGS) {
    globalQkmerFlag = 1;
    sequence *seq = PG_GETARG_SEQ_P(0);
    text *out = (text *)DirectFunctionCall1(textin, PointerGetDatum(seq_sequence_to_string(seq)));
    PG_FREE_IF_COPY(seq, 0);
    PG_RETURN_TEXT_P(out);
}
