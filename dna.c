#include "dna.h"
#include "sequence.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(dna_in);
Datum dna_in(PG_FUNCTION_ARGS) {
    // globalQkmerFlag = 1;
    char *str = PG_GETARG_CSTRING(0);
    sequence* seq = seq_string_to_sequence(str);
    PG_RETURN_SEQ_P(seq);
}

PG_FUNCTION_INFO_V1(dna_out);
Datum dna_out(PG_FUNCTION_ARGS) {
    // globalQkmerFlag = 1;
    sequence *seq = PG_GETARG_SEQ_P(0);
    char *result = seq_sequence_to_string(seq);
    PG_FREE_IF_COPY(seq, 0);
    PG_RETURN_CSTRING(result);
}

PG_FUNCTION_INFO_V1(dna_cast_from_text);
Datum dna_cast_from_text(PG_FUNCTION_ARGS) {
    // globalQkmerFlag = 1;
    text *txt = PG_GETARG_TEXT_P(0);
    char *str = DatumGetCString(DirectFunctionCall1(textout, PointerGetDatum(txt))); //MacOS
    //char *str = text_to_cstring(txt);
    PG_RETURN_SEQ_P(seq_string_to_sequence(&str));
}

PG_FUNCTION_INFO_V1(dna_cast_to_text);
Datum dna_cast_to_text(PG_FUNCTION_ARGS) {
    // globalQkmerFlag = 1;
    sequence *seq = PG_GETARG_SEQ_P(0);
    text *out = (text *)DirectFunctionCall1(textin, PointerGetDatum(seq_sequence_to_string(seq))); //MacOS
    //text *out = cstring_to_text(seq_sequence_to_string(seq));
    PG_FREE_IF_COPY(seq, 0);
    PG_RETURN_TEXT_P(out);
}
