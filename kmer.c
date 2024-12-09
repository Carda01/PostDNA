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
    PG_FREE_IF_COPY(kmer, 0);
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

PG_FUNCTION_INFO_V1(kmer_canonicalize);
Datum kmer_canonicalize(PG_FUNCTION_ARGS)
{
    sequence *kmer = (sequence *)PG_GETARG_POINTER(0);
    sequence *canonical = kmer_internal_canonicalize(kmer);
    PG_FREE_IF_COPY(kmer, 0);
    PG_RETURN_SEQ_P(canonical);
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


uint8_t kmer_get_base_at_index(const uint8_t* data, int index){
    int byte_index, overflow;
    byte_index = index / 4;
    overflow = index % 4;
    uint8_t shift = 6 - 2 * overflow;
    return (data[byte_index] >> shift) & BASE_MASK;
}

void kmer_set_base_at_index(uint8_t* data, int index, uint8_t base) {
  int byte_index = index / 4;
  int overflow = index % 4;
  uint8_t shift = 6 - 2 * overflow;
  uint8_t shifted_base = base << shift;
  uint8_t cleaner = ~(BASE_MASK << shift);
  data[byte_index] = (data[byte_index] & cleaner) | shifted_base;
}

sequence *kmer_internal_canonicalize(sequence *kmer){
    const size_t length = seq_get_length(kmer, KMER);
    sequence* reverse_complement = seq_create_empty_sequence(length, KMER);
    bool continue_building = false;
    for(int i = length - 1; i >= 0; i--) {
        uint8_t reverse_base = (~kmer_get_base_at_index(kmer->data, i) & BASE_MASK);
        uint8_t compare_base = kmer_get_base_at_index(kmer->data, length-1-i);
        if(!continue_building && compare_base < reverse_base) {
            pfree(reverse_complement);
            return kmer;
        }
        else if(reverse_base < compare_base) {
            continue_building = true;
        }
        kmer_set_base_at_index(reverse_complement->data, length - 1 - i, reverse_base);
    }
    
    return reverse_complement;
}


Datum
kmer_create_subseq(const uint8_t *data, int begin, int length)
{
    sequence* seq;
    const size_t num_bytes = seq_get_number_of_bytes_from_length(length, KMER);
    const uint8_t overflow = begin % 4;
    data += (begin / 4);
    if(overflow == 0){
        seq = seq_create_sequence(data, length, num_bytes, KMER);
    }
    else {
        uint8_t *new_data = (uint8_t *) palloc0(sizeof(uint8_t) * (num_bytes));
        int bits_copied = 0;
        for(int i = 0; bits_copied < length; i++){
            new_data[i] = data[i] << (overflow * 2);
            bits_copied += (4 - overflow);
            if(bits_copied < length){
                new_data[i] |= data[i + 1] >> ((4 - overflow)*2);
                bits_copied += overflow;
            }
        }
        seq = seq_create_sequence(new_data, length, num_bytes, KMER);
        pfree(new_data);
    }

    if (length % 4){ 
        uint8_t cleaner = 0x0;
        uint8_t i = 8 - 2 * (length % 4);
        while(i <= 6) {
            cleaner |= (BASE_MASK << i); 
            i+=2;
        }
        seq->data[num_bytes-1] &= cleaner;
    }

	return PointerGetDatum(seq);
}

void kmer_fill_copy(uint8_t* target, uint8_t* source, int target_start, int length) {
    int data_bytes = length / 4;
    int overflow = length % 4;
    int start_byte = target_start / 4;
    int start_overflow = target_start % 4;
    int bits_copied = 0;
    for (int i = 0; bits_copied < length; i++) {
        target[start_byte + i] |= source[i] >> (start_overflow * 2);
        bits_copied += 4 - start_overflow;
        if(bits_copied < length) {
            target[start_byte + i + 1] |= source[i] << ((4 - start_overflow) * 2);
            bits_copied += start_overflow;
        }
    }
    
    int final_length = target_start + 1 + length;
    int final_size = seq_get_number_of_bytes_from_length(final_length, KMER);
    if ((final_length) % 4){ 
        uint8_t cleaner = 0x0;
        uint8_t i = 8 - 2 * (final_length % 4);
        while(i <= 6) {
            cleaner |= (BASE_MASK << i); 
            i+=2;
        }
        target[final_size - 1] &= cleaner;
    }
}

