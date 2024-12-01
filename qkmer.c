#include "qkmer.h"
#include "sequence.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

PG_FUNCTION_INFO_V1(qkmer_length);
Datum qkmer_length(PG_FUNCTION_ARGS)
{
    sequence *qkmer = (sequence *)PG_GETARG_POINTER(0);
    uint8_t qkmer_length = seq_get_length(qkmer, QKMER);  // Get the length of the qkmer sequence
    PG_RETURN_UINT32(qkmer_length);
}

PG_FUNCTION_INFO_V1(qkmer_in);
Datum qkmer_in (PG_FUNCTION_ARGS) {
    char *str = PG_GETARG_CSTRING(0);
    sequence* seq = seq_string_to_sequence(str, QKMER);
    PG_RETURN_SEQ_P(seq);
}

PG_FUNCTION_INFO_V1(qkmer_out);
Datum qkmer_out(PG_FUNCTION_ARGS) {
    sequence *seq = PG_GETARG_SEQ_P(0);
    char *result = seq_sequence_to_string(seq, QKMER);
    PG_FREE_IF_COPY(seq, 0);
    PG_RETURN_CSTRING(result);
}


PG_FUNCTION_INFO_V1(qkmer_cast_from_text);
Datum qkmer_cast_from_text(PG_FUNCTION_ARGS) {
    text *txt = PG_GETARG_TEXT_P(0);
    char *str = DatumGetCString(DirectFunctionCall1(textout, PointerGetDatum(txt)));
    PG_RETURN_SEQ_P(seq_string_to_sequence(&str, QKMER));
}


PG_FUNCTION_INFO_V1(qkmer_cast_to_text);
Datum qkmer_cast_to_text(PG_FUNCTION_ARGS) {
    sequence *seq = PG_GETARG_SEQ_P(0);
    text *out = (text *)DirectFunctionCall1(textin, PointerGetDatum(seq_sequence_to_string(seq, QKMER)));
    PG_FREE_IF_COPY(seq, 0);
    PG_RETURN_TEXT_P(out);
}

PG_FUNCTION_INFO_V1(qkmer_equals);
Datum qkmer_equals(PG_FUNCTION_ARGS) {
  sequence *qkmer1 = PG_GETARG_SEQ_P(0);
  sequence *qkmer2 = PG_GETARG_SEQ_P(1);
  bool result = seq_equals(qkmer1, qkmer2, QKMER);
  PG_FREE_IF_COPY(qkmer1, 0);
  PG_FREE_IF_COPY(qkmer2, 1);
  PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(qkmer_starts_with);
Datum qkmer_starts_with(PG_FUNCTION_ARGS) {
  sequence *qkmer = PG_GETARG_SEQ_P(0);
  sequence *prefix = PG_GETARG_SEQ_P(1);
  bool result = seq_starts_with(qkmer, prefix, QKMER);
  PG_FREE_IF_COPY(qkmer, 0);
  PG_FREE_IF_COPY(prefix, 1);
  PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(qkmer_contains);
Datum qkmer_contains(PG_FUNCTION_ARGS) {
  sequence *qkmer = PG_GETARG_SEQ_P(0);
  sequence *kmer = PG_GETARG_SEQ_P(1);
  bool result = qkmer_contains_internal(qkmer, kmer);
  PG_FREE_IF_COPY(qkmer, 0);
  PG_FREE_IF_COPY(kmer, 1);
  PG_RETURN_BOOL(result);
}


/******************************************************************************/

bool qkmer_contains_internal(sequence* qkmer, sequence* kmer)
{

  const uint8_t len = seq_get_length(qkmer, QKMER);
  const uint8_t len_kmer = seq_get_length(kmer, KMER);
  if (len != len_kmer)
  {
      return false;
  }


  for (size_t i=0; i<len; ++i)
  {
  // Get the i-th query symbol.
      uint8_t shift = 4 - 4 * (i % 2);
      uint8_t mask = QUERY_MASK << shift;
      uint8_t symbol = (qkmer->data[i / 2] & mask) >> shift;
  
  // Get the i-th kmer base.
      uint8_t shift2 = 6 - 2 * (i % 4);
      uint8_t mask2 = BASE_MASK << shift2;
      uint8_t base = (kmer->data[i / 4] & mask2) >> shift2;
  
  if (symbol == BASE_N) // N matches to all bases
      {continue;}
  
  if ((symbol>>2 & BASE_MASK) == 0) // if it's a base symbol (A,C,G,T)
      {
          if (symbol != base)
              {   
                  return false;
              }
          else
              {continue;}
      }
  
  if ( !match_Qsymbol (symbol, base) ) 
      {
          return false;
      }
  
  }
  return true;
}



bool match_Qsymbol (uint8_t symbol, uint8_t kmer_base)
{

switch(symbol){
    case(BASE_R):
        return (kmer_base==BASE_A | kmer_base==BASE_G );
    case(BASE_Y):
        return (kmer_base==BASE_C | kmer_base==BASE_T );
    case(BASE_S):
        return (kmer_base==BASE_G | kmer_base==BASE_C );
    case(BASE_W):
        return (kmer_base==BASE_A | kmer_base==BASE_T );
    case(BASE_K):
        return (kmer_base==BASE_G | kmer_base==BASE_T );
    case(BASE_M):
        return (kmer_base==BASE_A | kmer_base==BASE_C );
    case(BASE_B):
        return (kmer_base != BASE_A );
    case(BASE_D):
        return (kmer_base != BASE_C );
    case(BASE_H):
        return (kmer_base != BASE_G );
    case(BASE_V):
        return (kmer_base != BASE_T );    
    default: return false; 
}

}
