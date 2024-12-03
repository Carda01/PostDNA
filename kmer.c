#include "kmer.h"
#include "sequence.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "utils/array.h"   // For ArrayType
#include "catalog/pg_type.h" // For type constants 
#include "utils/builtins.h" // For pg_strtoint32

PG_FUNCTION_INFO_V1(kmer_length);
Datum kmer_length(PG_FUNCTION_ARGS)
{
    sequence *kmer = (sequence *)PG_GETARG_POINTER(0);
    size_t kmer_length = seq_get_length(kmer, KMER);  // Get the length of the kmer sequence
    PG_RETURN_UINT32(kmer_length);
}

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

PG_FUNCTION_INFO_V1(kmer_starts_with);
Datum kmer_starts_with(PG_FUNCTION_ARGS) {
  sequence *kmer = PG_GETARG_SEQ_P(0);
  sequence *prefix = PG_GETARG_SEQ_P(1);
  bool result = seq_starts_with(kmer, prefix, KMER);
  PG_FREE_IF_COPY(kmer, 0);
  PG_FREE_IF_COPY(prefix, 1);
  PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(kmer_started_with);
Datum kmer_started_with(PG_FUNCTION_ARGS) {
  sequence *prefix = PG_GETARG_SEQ_P(0);
  sequence *kmer = PG_GETARG_SEQ_P(1);
  bool result = seq_starts_with(kmer, prefix, KMER);
  PG_FREE_IF_COPY(prefix, 0);
  PG_FREE_IF_COPY(kmer, 1);
  PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(kmer_hash);
Datum kmer_hash(PG_FUNCTION_ARGS) {
    sequence *seq = PG_GETARG_SEQ_P(0);
    int32 out = seq_hash(seq, KMER); 
    PG_FREE_IF_COPY(seq, 0);
    PG_RETURN_INT32(out);
}

PG_FUNCTION_INFO_V1(kmer_typmod_cast);
Datum kmer_typmod_cast(PG_FUNCTION_ARGS) {
    sequence *seq = (sequence *)PG_GETARG_POINTER(0);
    int typmod = PG_GETARG_INT32(1); // Type modifier

    // elog(NOTICE, "Typmod received in kmer_cast_from_text_typmod: %d", typmod);

    if (typmod >= 0) // If type modifier exists
    {
        int max_length = typmod; // Typmod is the maximum allowed length
        size_t seq_length = seq_get_length(seq, KMER);

        if (seq_length > max_length) {
            ereport(ERROR,
                    (errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
                     errmsg("kmer sequence exceeds max length of %d", max_length)));
        }
    }

    PG_RETURN_SEQ_P(seq);
}

PG_FUNCTION_INFO_V1(kmer_typmod_in);
Datum kmer_typmod_in(PG_FUNCTION_ARGS) {
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
                 errdetail("Kmer type modifier should be a single integer (e.g. kmer(10)). ")));
    }

    if ( typmod < 1 || typmod > 32 ) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("Invalid type modifier"),
                 errdetail("Length for kmer type modifier must be between 1 and 32. ")));
    }

    // elog(NOTICE, "Typmod parsed in kmer_typmod_in: %d", typmod);

    PG_RETURN_INT32(typmod); // Return the max length of kmer
}


PG_FUNCTION_INFO_V1(kmer_typmod_out);
Datum kmer_typmod_out(PG_FUNCTION_ARGS) {
    int typmod = PG_GETARG_INT32(0);
    char *result;

    if (typmod < 0) {
        result = pstrdup(""); // No modifier specified
    } else {
        result = psprintf("(%d)", typmod); // Format as (50), for example
    }

    PG_RETURN_CSTRING(result);
}
