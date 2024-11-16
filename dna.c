#include "dna.h"
#include "common.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>


size_t dna_get_length(DNA* dna) { return com_get_length(dna); }

DNA* dna_string_to_DNA(const char *dna_str) {
  const size_t dna_length = strlen(dna_str);
  const size_t num_bytes = com_get_number_of_bytes(dna_length);

  uint8_t* data = malloc(num_bytes);
  memset(data, 0, num_bytes);

  com_encode2(dna_str, data, dna_length);
  DNA *dna = (DNA *) palloc(sizeof(DNA) + num_bytes);
  SET_VARSIZE(dna, sizeof(DNA) + num_bytes);
  dna->overflow=(dna_length - ((num_bytes - 1) * 4)) % 4;
  memcpy(dna->data, data, num_bytes);

  free(data);
  return dna;
}

char *dna_DNA_to_string(DNA *dna) { 
    return com_decode(dna->data, com_get_length(dna));
}

bool dna_equals(DNA* dna1, DNA* dna2) {
  if (com_get_length(dna1) != com_get_length(dna2)) {
    return false;
  }

  for (size_t i = 0; i < com_get_length(dna1); ++i) {
    if (dna1->data[i] != dna2->data[i]) {
      return false;
    }
  }

  return true;
}

KMER *dna_generate_kmers(DNA* dna, uint8_t k) {
  if (k > 32) {
    printf("Invalid k size (>32)");
    return NULL;
  }

  const size_t num_kmers = com_get_num_generable_kmers(com_get_length(dna), k);
  const uint8_t data_bytes = com_get_number_of_bytes(k);
  KMER *kmers = malloc(sizeof(KMER) * num_kmers);
  for (size_t i = 0; i < num_kmers; ++i) {
    KMER kmer;
    kmer.k = k;
    kmer.data = malloc(sizeof(uint8_t) * data_bytes);
    size_t init_byte = (2 * i) / 8;
    uint8_t init_pos = (2 * i) % 8;
    uint8_t step = 2 * k - 1;
    for (size_t c = 0; c < data_bytes; ++c) {
      kmer.data[c] = dna->data[c + init_byte];
      kmer.data[c] <<= init_pos;
      if ((init_pos + step) / 8 != c) {
        kmer.data[c] |= dna->data[c + init_byte + 1] >> (8 - init_pos);
      }
    }
    kmers[i] = kmer;
  }

  return kmers;
}

/*****************************************************************************/

// PG_FUNCTION_INFO_V1(dna_DNA_in);
// Datum dna_DNA_in(PG_FUNCTION_ARGS) {
//   char *str = PG_GETARG_CSTRING(0);
//   PG_RETURN_DNA_P(dna_string_to_DNA(&str));
// }
// 
// PG_FUNCTION_INFO_V1(dna_DNA_out);
// Datum dna_DNA_out(PG_FUNCTION_ARGS) {
//   DNA *dna = PG_GETARG_DNA_P(0);
//   char *result = dna_DNA_to_string(dna);
//   PG_FREE_IF_COPY(dna, 0);
//   PG_RETURN_CSTRING(result);
// }

// PG_FUNCTION_INFO_V1(dna_DNA_cast_from_text);
// Datum dna_DNA_cast_from_text(PG_FUNCTION_ARGS) {
//   text *txt = PG_GETARG_TEXT_P(0);
//   char *str = text_to_cstring(txt);
//   // char *str =
//   //     DatumGetCString(DirectFunctionCall1(textout, PointerGetDatum(txt)));
//   
//   PG_RETURN_DNA_P(dna_string_to_DNA(&str));
// }
// 
// PG_FUNCTION_INFO_V1(dna_DNA_cast_to_text);
// Datum dna_DNA_cast_to_text(PG_FUNCTION_ARGS) {
//   DNA *dna = PG_GETARG_DNA_P(0);
//   text *out = cstring_to_text(dna_DNA_to_string(dna));
//   // text *out =
//   //     (text *)DirectFunctionCall1(textin, PointerGetDatum(dna_DNA_to_string(dna)));
//   PG_FREE_IF_COPY(dna, 0);
//   PG_RETURN_TEXT_P(out);
// }

/*****************************************************************************/
