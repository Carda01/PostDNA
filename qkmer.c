#include "qkmer.h"
#include "sequence.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>


// TODO add condition on length < 32
PG_FUNCTION_INFO_V1(qkmer_in);
Datum qkmer_in (PG_FUNCTION_ARGS) {
    // globalQkmerFlag = 2;
    char *str = PG_GETARG_CSTRING(0);
    sequence* seq = qkmer_string_to_sequence(str);
    PG_RETURN_SEQ_P(seq);
}

PG_FUNCTION_INFO_V1(qkmer_out);
Datum qkmer_out(PG_FUNCTION_ARGS) {
    // globalQkmerFlag = 2;
    sequence *seq = PG_GETARG_SEQ_P(0);
    char *result = qkmer_sequence_to_string(seq);
    PG_FREE_IF_COPY(seq, 0);
    PG_RETURN_CSTRING(result);
}

// TODO add condition on length < 32
PG_FUNCTION_INFO_V1(qkmer_cast_from_text);
Datum qkmer_cast_from_text(PG_FUNCTION_ARGS) {
    // globalQkmerFlag = 2;
    text *txt = PG_GETARG_TEXT_P(0);
    char *str = DatumGetCString(DirectFunctionCall1(textout, PointerGetDatum(txt))); //MacOS
    // char *str = text_to_cstring(txt);
    PG_RETURN_SEQ_P(qkmer_string_to_sequence(&str));
}

PG_FUNCTION_INFO_V1(qkmer_cast_to_text);
Datum qkmer_cast_to_text(PG_FUNCTION_ARGS) {
    // globalQkmerFlag = 2;
    sequence *seq = PG_GETARG_SEQ_P(0);
    text *out = (text *)DirectFunctionCall1(textin, PointerGetDatum(qkmer_sequence_to_string(seq))); //MacOS
    // text *out = cstring_to_text(seq_sequence_to_string(seq));
    PG_FREE_IF_COPY(seq, 0);
    PG_RETURN_TEXT_P(out);
}


sequence *qkmer_string_to_sequence(const char *qkmer_str)
{

    const size_t seq_length = strlen(qkmer_str);
    size_t num_bytes;
    uint8_t* data = qkmer_encode(qkmer_str, seq_length, &num_bytes);

    sequence *seq = (sequence *) palloc(sizeof(sequence) + num_bytes);
    SET_VARSIZE(seq, sizeof(sequence) + num_bytes);
    seq->overflow = seq_length % 2;
    memcpy(seq->data, data, num_bytes);

    free(data);
    return seq;

}

uint8_t *qkmer_encode(const char *qkmer_str, const size_t sequence_len, size_t *data_bytes)
{

  *data_bytes = (sequence_len / 2) + (sequence_len % 2 != 0); // 1 byte = 2 letters
  uint8_t *data = (uint8_t *) malloc(sizeof(uint8_t) * (*data_bytes));
  memset(data, 0, (*data_bytes));

  for (size_t i = 0; i < sequence_len; ++i) {
    uint8_t shift = 4 - 4 * (i % 2);

    switch (qkmer_str[i]) {
    case 'A':
      data[i / 2] |= BASE_A << shift;
      break;
    case 'C':
      data[i / 2] |= BASE_C << shift;
      break;
    case 'G':
      data[i / 2] |= BASE_G << shift;
      break;
    case 'T':
      data[i / 2] |= BASE_T << shift;
      break;
    case 'R':
      data[i / 2] |= BASE_R << shift;
      break;
    case 'Y':
      data[i / 2] |= BASE_Y << shift;
      break;
    case 'S':
      data[i / 2] |= BASE_S << shift;
      break;
    case 'W':
      data[i / 2] |= BASE_W << shift;
      break;
    case 'K':
      data[i / 2] |= BASE_K << shift;
      break;
    case 'M':
      data[i / 2] |= BASE_M << shift;
      break;
    case 'B':
      data[i / 2] |= BASE_B << shift;
      break;
    case 'D':
      data[i / 2] |= BASE_D << shift;
      break;
    case 'H':
      data[i / 2] |= BASE_H << shift;
      break;
    case 'V':
      data[i / 2] |= BASE_V << shift;
      break;
    case 'N':
      data[i / 2] |= BASE_N << shift;
      break;
    default:
      ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
        errmsg("Invalid QKmer Symbol: character number %d is '%c' which is not a valid Query Symbol", i, qkmer_str[i])));    }
  }
  return data;
}


char *qkmer_sequence_to_string(sequence *qkmer_seq)
{
  return qkmer_decode(qkmer_seq->data, qkmer_get_length(qkmer_seq));

}

char *qkmer_decode(uint8_t* qkmer_seq, size_t sequence_len)
{
char *sequence = palloc(sizeof(char) * (sequence_len + 1));

  for (size_t i = 0; i < sequence_len; ++i) {
    uint8_t shift = 4 - 4 * (i % 2);
    uint8_t mask = QUERY_MASK << shift;

    // Get the i-th symbol.
    uint8_t symbol = (qkmer_seq[i / 2] & mask) >> shift;

    switch (symbol) {
    case BASE_A:
      sequence[i] = 'A';
      break;
    case BASE_C:
      sequence[i] = 'C';
      break;
    case BASE_G:
      sequence[i] = 'G';
      break;
    case BASE_T:
      sequence[i] = 'T';
      break;
    case BASE_R:
      sequence[i] = 'R';
      break;
    case BASE_Y:
      sequence[i] = 'Y';
      break;
    case BASE_S:
      sequence[i] = 'S';
      break;
    case BASE_W:
      sequence[i] = 'W';
      break;
    case BASE_K:
      sequence[i] = 'K';
      break;
    case BASE_M:
      sequence[i] = 'M';
      break;
    case BASE_B:
      sequence[i] = 'B';
      break;
    case BASE_D:
      sequence[i] = 'D';
      break;
    case BASE_H:
      sequence[i] = 'H';
      break;
    case BASE_V:
      sequence[i] = 'V';
      break;
    case BASE_N:
      sequence[i] = 'N';
      break;
    default:
      ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
        errmsg("Invalid QKmer Symbol: character number %d is not valid!"), i));    
    }
  }

  sequence[sequence_len] = '\0';
  return sequence;
}



uint8_t qkmer_get_length(sequence* qkmer_seq)
{
  return (VARSIZE(qkmer_seq) - sizeof(sequence) - 1) * 2 + (qkmer_seq->overflow == 0 ? 2 : qkmer_seq->overflow);
}



// bool qkmer_contains(sequence qkmer, sequence kmer)
// {

// if (qkmer.k != kmer.k)
// {
//     printf("Lengths not equal!\n");
//     return 0;
// }

// uint8_t len = qkmer.k;

// for (size_t i=0; i<len; ++i)
// {
// // Get the i-th query symbol.
//     uint8_t shift = 4 - 4 * (i % 2);
//     uint8_t mask = QUERY_MASK << shift;
//     uint8_t symbol = (qkmer.data[i / 2] & mask) >> shift;

// // Get the i-th kmer base.
//     uint8_t shift2 = 6 - 2 * (i % 4);
//     uint8_t mask2 = BASE_MASK << shift2;
//     uint8_t base = (kmer.data[i / 4] & mask2) >> shift2;

// if (symbol == BASE_N) // N matches to all bases
//     {continue;}

// if ((symbol>>2 & BASE_MASK) == 0) // if it's a base symbol (A,C,G,T)
//     {
//         if (symbol != base)
//             {   
//                 printf("No match at position %i\n",i);
//                 return 0;
//             }
//         else
//             {continue;}
//     }

// if ( !match_Qsymbol (symbol, base) ) 
//     {
//         printf("No match at position %i\n",i);
//         return 0;
//     }

// }

// return 1;

// }



// bool match_Qsymbol (uint8_t symbol, uint8_t kmer_base)
// {

// switch(symbol){
//     case(BASE_R):
//         return (kmer_base==BASE_A | kmer_base==BASE_G );
//     case(BASE_Y):
//         return (kmer_base==BASE_C | kmer_base==BASE_T );
//     case(BASE_S):
//         return (kmer_base==BASE_G | kmer_base==BASE_C );
//     case(BASE_W):
//         return (kmer_base==BASE_A | kmer_base==BASE_T );
//     case(BASE_K):
//         return (kmer_base==BASE_G | kmer_base==BASE_T );
//     case(BASE_M):
//         return (kmer_base==BASE_A | kmer_base==BASE_C );
//     case(BASE_B):
//         return (kmer_base != BASE_A );
//     case(BASE_D):
//         return (kmer_base != BASE_C );
//     case(BASE_H):
//         return (kmer_base != BASE_G );
//     case(BASE_V):
//         return (kmer_base != BASE_T );    
//     default: return 0; 
// }

// }