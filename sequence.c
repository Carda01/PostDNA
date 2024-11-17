#include "sequence.h"
#include <stdio.h>
#include <string.h>

int globalQkmerFlag = 0;

sequence* seq_string_to_sequence(const char *seq_str) {
    const size_t seq_length = strlen(seq_str);
    size_t num_bytes;
    uint8_t* data = seq_encode(seq_str, seq_length, &num_bytes);

    sequence *seq = (sequence *) palloc(sizeof(sequence) + num_bytes);
    SET_VARSIZE(seq, sizeof(sequence) + num_bytes);
    // seq->overflow = (seq_length - ((num_bytes - 1) * 4)) % 4;
    seq->overflow = seq_length % (4/globalQkmerFlag);

    memcpy(seq->data, data, num_bytes);

    free(data);
    return seq;
}

char *seq_sequence_to_string(sequence *seq) {
    return seq_decode(seq->data, seq_get_length(seq));
}

size_t seq_get_number_of_bytes(size_t seq_len) {
  return (seq_len / (4/globalQkmerFlag)) + (seq_len % (4/globalQkmerFlag) != 0);
}

size_t seq_get_length(sequence* seq){
  return (VARSIZE(seq) - sizeof(sequence) - 1) * (4/globalQkmerFlag) + (seq->overflow == 0 ? (4/globalQkmerFlag) : seq->overflow);
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
    uint8_t shift = globalQkmerFlag == 2 ? 4 - 4 * (i % 2) : 6 - 2 * (i % 4);
    uint8_t byte_index = i / (4/globalQkmerFlag);

    switch (toupper(seq_str[i])) {
    case 'A':
      data[byte_index] |= BASE_A << shift;
      break;
    case 'C':
      data[byte_index] |= BASE_C << shift;
      break;
    case 'G':
      data[byte_index] |= BASE_G << shift;
      break;
    case 'T':
      data[byte_index] |= BASE_T << shift;
      break;
    default:
      if(globalQkmerFlag == 2)
        switch (seq_str[i]) {
        case 'R':
          data[byte_index] |= BASE_R << shift;
          break;
        case 'Y':
          data[byte_index] |= BASE_Y << shift;
          break;
        case 'S':
          data[byte_index] |= BASE_S << shift;
          break;
        case 'W':
          data[byte_index] |= BASE_W << shift;
          break;
        case 'K':
          data[byte_index] |= BASE_K << shift;
          break;
        case 'M':
          data[byte_index] |= BASE_M << shift;
          break;
        case 'B':
          data[byte_index] |= BASE_B << shift;
          break;
        case 'D':
          data[byte_index] |= BASE_D << shift;
          break;
        case 'H':
          data[byte_index] |= BASE_H << shift;
          break;
        case 'V':
          data[byte_index] |= BASE_V << shift;
          break;
        case 'N':
          data[byte_index] |= BASE_N << shift;
          break;
        default:
          ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
            errmsg("Invalid DNA base: character number %d is '%c' which is not a valid DNA base", i, seq_str[i])));

        }
      else
        ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
          errmsg("Invalid DNA base: character number %d is '%c' which is not a valid DNA base", i, seq_str[i])));
    }
  }
  return data;
}


char* seq_decode(uint8_t* data, size_t sequence_len){
  char *sequence = palloc(sizeof(char) * (sequence_len + 1));

  for (size_t i = 0; i < sequence_len; ++i) {
    uint8_t shift = globalQkmerFlag == 2 ? 4 - 4 * (i % 2) : 6 - 2 * (i % 4);
    uint8_t mask = globalQkmerFlag == 2 ? QUERY_MASK << shift : BASE_MASK << shift;

    // Get the i-th DNA base.
    uint8_t base = (data[i / (4/globalQkmerFlag)] & mask) >> shift;

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
      if(globalQkmerFlag == 2)
        switch (base) {
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
      else
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
