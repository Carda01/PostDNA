#include "sequence.h"
#include <stdio.h>
#include <string.h>

sequence* seq_string_to_sequence(const char *seq_str) {
    const size_t seq_length = strlen(seq_str);
    size_t num_bytes;
    uint8_t* data = seq_encode(seq_str, seq_length, &num_bytes);

    sequence *seq = (sequence *) palloc(sizeof(sequence) + num_bytes);
    SET_VARSIZE(seq, sizeof(sequence) + num_bytes);
    seq->overflow = (seq_length - ((num_bytes - 1) * 4)) % 4;
    memcpy(seq->data, data, num_bytes);

    free(data);
    return seq;
}

char *seq_sequence_to_string(sequence *seq) {
    return seq_decode(seq->data, seq_get_length(seq));
}

size_t seq_get_number_of_bytes(size_t seq_len) {
  return (seq_len / 4) + (seq_len % 4 != 0);
}

size_t seq_get_length(sequence* seq){
  return (VARSIZE(seq) - sizeof(sequence) - 1) * 4 + (seq->overflow == 0 ? 4 : seq->overflow);
}

uint8_t seq_get_overflow(size_t seq_length, size_t num_bytes) {
    return (seq_length - ((num_bytes - 1) * 4)) % 4;
}

// TODO helper used in debug, allow it to work with elog
void seq_print_binary(const uint8_t value) {
  for (int i = 7; i >= 0; i--) {
    printf("%d", (value >> i) & 1);
  }
  printf("\n");
}


uint8_t *seq_encode(const char *seq_str, const size_t sequence_len, size_t *data_bytes) {
  *data_bytes = seq_get_number_of_bytes(sequence_len);
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


char* seq_decode(uint8_t* data, size_t sequence_len){
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


size_t seq_get_num_generable_kmers(size_t seq_len, uint8_t k){
  return seq_len - k + 1;
}

bool seq_equals(sequence* seq1, sequence* seq2) {
    if (seq_get_length(seq1) != seq_get_length(seq2)) {
        return false;
    }

    for (size_t i = 0; i < seq_get_length(seq1); ++i) {
        if (seq1->data[i] != seq2->data[i]) {
            return false;
        }
    }

    return true;
}

// TODO rework it to generate variable-length kmers
//sequence *seq_generate_kmers(sequence* seq, uint8_t k) {
//    if (k > 32) {
//        printf("Invalid k size (>32)");
//        return NULL;
//    }
//
//    const size_t num_kmers = seq_get_num_generable_kmers(seq_get_length(seq), k);
//    const uint8_t data_bytes = seq_get_number_of_bytes(k);
//    sequence *kmers = malloc(sizeof(sequence) * num_kmers);
//    for (size_t i = 0; i < num_kmers; ++i) {
//        sequence kmer;
//        kmer.k = k;
//        kmer.data = malloc(sizeof(uint8_t) * data_bytes);
//        size_t init_byte = (2 * i) / 8;
//        uint8_t init_pos = (2 * i) % 8;
//        uint8_t step = 2 * k - 1;
//        for (size_t c = 0; c < data_bytes; ++c) {
//            kmer.data[c] = seq->data[c + init_byte];
//            kmer.data[c] <<= init_pos;
//            if ((init_pos + step) / 8 != c) {
//                kmer.data[c] |= seq->data[c + init_byte + 1] >> (8 - init_pos);
//            }
//        }
//        kmers[i] = kmer;
//    }
//
//    return kmers;
//}
