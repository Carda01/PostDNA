#include "qkmer.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

uint8_t qkmer_get_length(QKMER qkmer)
{
return qkmer.k;
}


QKMER qkmer_string_to_QKMER(const char *qkmer_str)
{
  QKMER qkmer;
  size_t str_len = strlen(qkmer_str);
  if (str_len > 32) {
    printf("Invalid size for qkmer (it can't be greater than 32)\n");
    qkmer.k = -1;
    return qkmer;
  }
  qkmer.k = strlen(qkmer_str);
  qkmer.data = qkmer_encode(qkmer_str, qkmer.k);
  return qkmer;

}

uint8_t *qkmer_encode(const char *qkmer_str, uint8_t k)
{

  const size_t data_bytes = (k / 2) + (k % 2 != 0); // 1 byte = 2 letters
  uint8_t *data = malloc(sizeof(uint8_t) * data_bytes);
  memset(data, 0, data_bytes);

  for (size_t i = 0; i < k; ++i) {
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
      printf("Invalid Query Symbol");
    }
    // com_print_binary(data[i / 2]);
  }
  return data;
}



char *qkmer_decode(uint8_t *qkmer_seq, uint8_t k)
{

char *sequence = malloc(sizeof(char) * (k + 1));

  for (size_t i = 0; i < k; ++i) {
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
      printf("Invalid Query Symbol");
    }
  }

  sequence[k] = '\0';
  return sequence;
}


char *qkmer_QKMER_to_string(QKMER qkmer)
{
  return qkmer_decode(qkmer.data, qkmer.k);

}


bool qkmer_contains(QKMER qkmer, KMER kmer)
{

if (qkmer.k != kmer.k)
{
    printf("Lengths not equal!\n");
    return 0;
}

uint8_t len = qkmer.k;

for (size_t i=0; i<len; ++i)
{
// Get the i-th query symbol.
    uint8_t shift = 4 - 4 * (i % 2);
    uint8_t mask = QUERY_MASK << shift;
    uint8_t symbol = (qkmer.data[i / 2] & mask) >> shift;

// Get the i-th kmer base.
    uint8_t shift2 = 6 - 2 * (i % 4);
    uint8_t mask2 = BASE_MASK << shift2;
    uint8_t base = (kmer.data[i / 4] & mask2) >> shift2;

if (symbol == BASE_N) // N matches to all bases
    {continue;}

if ((symbol>>2 & BASE_MASK) == 0) // if it's a base symbol (A,C,G,T)
    {
        if (symbol != base)
            {   
                printf("No match at position %i\n",i);
                return 0;
            }
        else
            {continue;}
    }

if ( !match_Qsymbol (symbol, base) ) 
    {
        printf("No match at position %i\n",i);
        return 0;
    }

}

return 1;

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
    default: return 0; 
}

}