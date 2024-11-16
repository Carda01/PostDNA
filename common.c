#include "common.h"
#include <stdio.h>
#include <string.h>

size_t com_get_number_of_bytes(size_t dna_len) {
  return (dna_len / 4) + (dna_len % 4 != 0);
}

size_t com_get_length(DNA* dna){
  return (VARSIZE(dna) - sizeof(DNA) - 1) * 4 + (dna->overflow == 0 ? 4 : dna->overflow);
}

void com_print_binary(const uint8_t value) {
  for (int i = 7; i >= 0; i--) {
    printf("%d", (value >> i) & 1);
  }
  printf("\n");
}


uint8_t *com_encode(const char *seq_str, const size_t sequence_len, size_t *data_bytes) {
  *data_bytes = com_get_number_of_bytes(sequence_len);
  uint8_t *data = (uint8_t *) malloc(sizeof(uint8_t) * (*data_bytes));
  memset(data,0,(*data_bytes));

  for (size_t i = 0; i < sequence_len; ++i) {
    uint8_t shift = 6 - 2 * (i % 4);

    switch (toupper(seq_str[i])) {
    case 'A':
      data[i / 4] |= BASE_A << shift;
      break;
    case 'C':
      data[i / 4] |= BASE_C << shift;
      break;
    case 'G':
      data[i / 4] |= BASE_G << shift;
      break;
    case 'T':
      data[i / 4] |= BASE_T << shift;
      break;
    default:
      ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
        errmsg("Invalid DNA base: character number %d is '%c' which is not a valid DNA base", i, seq_str[i])));
    }
  }
  return data;
}


char* com_decode(uint8_t* data, size_t sequence_len){
  char *sequence = palloc(sizeof(char) * (sequence_len + 1));

  for (size_t i = 0; i < sequence_len; ++i) {
    uint8_t shift = 6 - 2 * (i % 4);
    uint8_t mask = BASE_MASK << shift;

    // Get the i-th DNA base.
    uint8_t base = (data[i / 4] & mask) >> shift;

    switch (base) {
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
    default:
      ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
        errmsg("Decode failed, character number %d is corrupted"), i));
    }
  }

  sequence[sequence_len] = '\0';
  return sequence;
}


size_t com_get_num_generable_kmers(size_t dna_len, uint8_t k){
  return dna_len - k + 1;
}

