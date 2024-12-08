#include "dna.h"
#include "sequence.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "utils/array.h"   // For ArrayType
#include "catalog/pg_type.h" // For type constants 
#include "utils/builtins.h" // For pg_strtoint32

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(dna_length);
Datum dna_length(PG_FUNCTION_ARGS)
{
    sequence *dna = (sequence *)PG_GETARG_POINTER(0);
    size_t length = seq_get_length(dna, DNA);  
    PG_FREE_IF_COPY(dna, 0);
    PG_RETURN_UINT32(length);
}

PG_FUNCTION_INFO_V1(dna_in);
Datum dna_in(PG_FUNCTION_ARGS) {
    char *str = PG_GETARG_CSTRING(0);
    sequence* seq = seq_string_to_sequence(str, DNA);
    PG_RETURN_SEQ_P(seq);
}

PG_FUNCTION_INFO_V1(dna_out);
Datum dna_out(PG_FUNCTION_ARGS) {
    sequence *seq = PG_GETARG_SEQ_P(0);
    char *result = seq_sequence_to_string(seq, DNA);
    PG_FREE_IF_COPY(seq, 0);
    PG_RETURN_CSTRING(result);
}

PG_FUNCTION_INFO_V1(dna_cast_from_text);
Datum dna_cast_from_text(PG_FUNCTION_ARGS) {
    text *txt = PG_GETARG_TEXT_P(0);
    char *str = DatumGetCString(DirectFunctionCall1(textout, PointerGetDatum(txt)));
    PG_RETURN_SEQ_P(seq_string_to_sequence(&str, DNA));
}

PG_FUNCTION_INFO_V1(dna_cast_to_text);
Datum dna_cast_to_text(PG_FUNCTION_ARGS) {
    sequence *seq = PG_GETARG_SEQ_P(0);
    text *out = (text *)DirectFunctionCall1(textin, PointerGetDatum(seq_sequence_to_string(seq, DNA)));
    PG_FREE_IF_COPY(seq, 0);
    PG_RETURN_TEXT_P(out);
}

PG_FUNCTION_INFO_V1(dna_typmod_cast);
Datum dna_typmod_cast(PG_FUNCTION_ARGS) {
    sequence *seq = (sequence *)PG_GETARG_POINTER(0);
    int typmod = PG_GETARG_INT32(1); // Type modifier

    // elog(NOTICE, "Typmod received in dna_cast_from_text_typmod: %d", typmod);

    if (typmod >= 0) //if type modifier exists
    {
        int max_length = typmod; // Typmod is the maximum allowed length
        size_t seq_length = seq_get_length(seq, DNA);

        if (seq_length > max_length) {
            ereport(ERROR,
                    (errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
                     errmsg("DNA sequence exceeds max length of %d", max_length)));
        }
    }

    PG_RETURN_SEQ_P(seq);
}

PG_FUNCTION_INFO_V1(dna_typmod_in);
Datum dna_typmod_in(PG_FUNCTION_ARGS) {
    ArrayType *modifiers = PG_GETARG_ARRAYTYPE_P(0);
    int32 *mods;
    int nmods;

    // Get the modifiers and their count
    mods = ArrayGetIntegerTypmods(modifiers, &nmods);

    int typmod = mods[0];

    if (nmods != 1) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("Invalid type modifier"),
                 errdetail("DNA type modifier should be a single integer (e.g. dna(10)).")));
    }

    if (typmod < 1) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("Invalid type modifier"),
                 errdetail("Length for DNA type modifier must be a positive integer.")));
    }

    // elog(NOTICE, "Typmod parsed in dna_typmod_in: %d", typmod);

    PG_RETURN_INT32(typmod); // Return the max length of DNA
}


PG_FUNCTION_INFO_V1(dna_typmod_out);
Datum dna_typmod_out(PG_FUNCTION_ARGS) {
    int typmod = PG_GETARG_INT32(0);
    char *result;

    if (typmod < 0) {
        result = pstrdup(""); // No modifier specified
    } else {
        result = psprintf("(%d)", typmod); // Format as (50), for example
    }

    PG_RETURN_CSTRING(result);
}
