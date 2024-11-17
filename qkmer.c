#include "qkmer.h"
#include "sequence.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>


PG_FUNCTION_INFO_V1(qkmer_in);
Datum qkmer_in (PG_FUNCTION_ARGS) {
    globalQkmerFlag = 2;
    char *str = PG_GETARG_CSTRING(0);
    if(strlen(str)>32)
        ereport(ERROR, (errcode(ERRCODE_NAME_TOO_LONG), errmsg("QKmer Length should be less than or equal 32.")));
    sequence* seq = seq_string_to_sequence(str);

    PG_RETURN_SEQ_P(seq);
}

PG_FUNCTION_INFO_V1(qkmer_out);
Datum qkmer_out(PG_FUNCTION_ARGS) {
    globalQkmerFlag = 2;
    sequence *seq = PG_GETARG_SEQ_P(0);
    char *result = seq_sequence_to_string(seq);
    PG_FREE_IF_COPY(seq, 0);
    PG_RETURN_CSTRING(result);
}

PG_FUNCTION_INFO_V1(qkmer_cast_from_text);
Datum qkmer_cast_from_text(PG_FUNCTION_ARGS) {
    globalQkmerFlag = 2;
    text *txt = PG_GETARG_TEXT_P(0);
    char *str = DatumGetCString(DirectFunctionCall1(textout, PointerGetDatum(txt))); //MacOS
    // char *str = text_to_cstring(txt);
    if(strlen(str)>32)
        ereport(ERROR, (errcode(ERRCODE_NAME_TOO_LONG), errmsg("QKmer Length should be less than or equal 32.")));
    PG_RETURN_SEQ_P(seq_string_to_sequence(&str));

}

PG_FUNCTION_INFO_V1(qkmer_cast_to_text);
Datum qkmer_cast_to_text(PG_FUNCTION_ARGS) {
    globalQkmerFlag = 2;
    sequence *seq = PG_GETARG_SEQ_P(0);
    // text *out = (text *)DirectFunctionCall1(textin, PointerGetDatum(qkmer_sequence_to_string(seq))); //MacOS
    text *out = (text *)DirectFunctionCall1(textin, PointerGetDatum(seq_sequence_to_string(seq))); //MacOS
    PG_FREE_IF_COPY(seq, 0);
    PG_RETURN_TEXT_P(out);
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