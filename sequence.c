#include "sequence.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "funcapi.h"
#include "catalog/pg_type.h"
#include "funcapi.h"


inline char* seq_get_string_type(int type) {
    return type == DNA ? "DNA" : type == KMER ? "KMER" : "QKMER";
}


sequence* seq_create_empty_sequence(size_t seq_length, int type) {
    size_t num_bytes = seq_get_number_of_bytes_from_length(seq_length, type);
    sequence *seq = (sequence *) palloc0(sizeof(sequence) + num_bytes);
    SET_VARSIZE(seq, sizeof(sequence) + num_bytes);
    seq->overflow = seq_get_overflow(seq_length, type);

    uint8_t *data = (uint8_t *) palloc0(sizeof(uint8_t) * (num_bytes));
    memcpy(seq->data, data, num_bytes);

    pfree(data);
    return seq;
}


sequence* seq_create_sequence(const uint8_t* data, size_t seq_length, size_t num_bytes, int type) {
    sequence *seq = (sequence *) palloc0(sizeof(sequence) + num_bytes);
    SET_VARSIZE(seq, sizeof(sequence) + num_bytes);
    seq->overflow = seq_get_overflow(seq_length, type);

    memcpy(seq->data, data, num_bytes);

    return seq;
}


sequence* seq_string_to_sequence(const char *seq_str, int type) {
    const size_t seq_length = strlen(seq_str);

    if(type != DNA && seq_length > KMER_MAX_SIZE){
        ereport(ERROR, (errcode(ERRCODE_NAME_TOO_LONG), errmsg("%s length should be less than or equal 32.", seq_get_string_type(type))));
    }

    size_t num_bytes;
    uint8_t* data = seq_encode(seq_str, seq_length, &num_bytes, type);

    sequence *seq = seq_create_sequence(data, seq_length, num_bytes, type);

    pfree(data);
    return seq;
}

inline int seq_bases_per_byte(int type) {
  return type == QKMER ? 2 : 4;
}

char *seq_sequence_to_string(sequence *seq, int type) {
    return seq_decode(seq->data, seq_get_length(seq, type), type);
}

size_t seq_get_number_of_bytes_from_length(size_t seq_len, int type) {
  return (seq_len / seq_bases_per_byte(type)) + (seq_get_overflow(seq_len, type) != 0);
}

size_t seq_get_number_of_occupied_bytes(sequence* seq) {
  return VARSIZE(seq) - sizeof(sequence);
}

size_t seq_get_length(sequence* seq, int type){
  return (VARSIZE(seq) - sizeof(sequence) - sizeof(uint8_t)) * seq_bases_per_byte(type) + (seq->overflow == 0 ? seq_bases_per_byte(type) : seq->overflow);
}

uint8_t seq_get_overflow(size_t seq_length, int type) {
    return seq_length % (seq_bases_per_byte(type));
}

// Helper used in debug, remember to free memory
char* seq_get_byte_binary_representation(const uint8_t value) {
  char* binary_representation = palloc0(9);
  for (int i = 7; i >= 0; i--) {
    binary_representation[7-i] = '0' + ((value >> i) & 1);
  }
  binary_representation[8] = '\0';
  return binary_representation;
}


uint8_t *seq_encode(const char *seq_str, const size_t sequence_len, size_t *data_bytes, int type) {
  *data_bytes = seq_get_number_of_bytes_from_length(sequence_len, type);
  uint8_t *data = (uint8_t *) palloc0(sizeof(uint8_t) * (*data_bytes));
  memset(data,0,(*data_bytes));

  for (size_t i = 0; i < sequence_len; ++i) {
    int bits_occupied = type == QKMER ? 4 : 2;
    uint8_t shift = (8 - bits_occupied) - bits_occupied * (i % seq_bases_per_byte(type));
    uint8_t byte_index = i / seq_bases_per_byte(type);

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
      if(type == QKMER){
        switch (toupper(seq_str[i])) {
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
            errmsg("Invalid QKMER base: character number %d is '%c' which is not a valid QKMER base", i, seq_str[i])));

        }
      }
      else {
        ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
          errmsg("Invalid %s base: character number %d is '%c' which is not a valid base", seq_get_string_type(type), i, seq_str[i])));
      }
    }
  }
  return data;
}


char* seq_decode(uint8_t* data, size_t sequence_len, int type){

  char *sequence = palloc0(sizeof(char) * (sequence_len + 1));

  for (size_t i = 0; i < sequence_len; ++i) {

    int bits_occupied = type == QKMER ? 4 : 2;
    uint8_t shift = (8 - bits_occupied) - bits_occupied * (i % seq_bases_per_byte(type));
    uint8_t mask = type == QKMER ? QUERY_MASK << shift : BASE_MASK << shift;

    // Get the i-th DNA base.
    uint8_t base = (data[i / seq_bases_per_byte(type)] & mask) >> shift;

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
      if(type == QKMER){
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
            errmsg("Decode failed for QKMER Symbol: character number %d is corrupted"), i));    
        }
      }
      else{
          ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
            errmsg("Decode failed for %s symbol: character number %d is corrupted"), seq_get_string_type(type), i));
      }
    }
  }

  sequence[sequence_len] = '\0';
  return sequence;
}


size_t seq_get_num_generable_kmers(size_t seq_len, uint8_t k){
  return seq_len - k + 1;
}

bool seq_equals(sequence* seq1, sequence* seq2, int type) {
    if (seq_get_length(seq1, type) != seq_get_length(seq2, type)) {
        return false;
    }

    const size_t num_bytes = seq_get_number_of_occupied_bytes(seq1);
    for (size_t i = 0; i < num_bytes; ++i) {
        if (seq1->data[i] != seq2->data[i]) {
            return false;
        }
    }

    return true;
}

bool seq_starts_with(sequence* seq, sequence* prefix, int type) {
    //prefix is the first argument
    const size_t prefix_length = seq_get_length(prefix, type);
    const size_t seq_length = seq_get_length(seq, type);

    if (prefix_length > seq_length) {
        return false;
    }

    const size_t num_bytes_prefix = seq_get_number_of_occupied_bytes(prefix);
    const size_t num_full_bytes_prefix = num_bytes_prefix - (seq_get_overflow(prefix_length, type) != 0 ? 1 : 0);
    // full bytes comparison
    for (size_t i = 0; i < num_full_bytes_prefix; ++i) {
        if (prefix->data[i] != seq->data[i]) {
            return false;
        }
    }

    // if the prefix has overflow, we should compare the significant bits of the prefix's last byte with the corresponding masked sequence's byte
    uint8_t prefix_overflow = seq_get_overflow(prefix_length, type);
    if (prefix_overflow > 0) {
        uint8_t mask = (0xFF << (8 - prefix_overflow * (type == QKMER ? 4 : 2)));
        uint8_t seq_corresponding_byte = seq->data[num_full_bytes_prefix] & mask;

        if (prefix->data[num_full_bytes_prefix] != seq_corresponding_byte) {
            return false;
        }
    }

    return true;
}

int seq_hash(sequence* seq, int type) {
  const size_t num_bytes = seq_get_number_of_occupied_bytes(seq);
  int hash = 0;
  for (int i = 0; i < num_bytes; i++){
    hash += seq->data[i];
  }
  return hash;
}

PG_FUNCTION_INFO_V1(generate_kmers);
Datum generate_kmers(PG_FUNCTION_ARGS)
{
    FuncCallContext     *funcctx; // context for all info we need accross calls
    int                  call_cntr;
    int                  max_calls;
 
    sequence *dna = (sequence *)PG_GETARG_POINTER(0);
    size_t k = PG_GETARG_INT32(1); // k (for generating k-mers)

    if (k>KMER_MAX_SIZE || k> seq_get_length(dna,DNA) || k<=0){
          ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
            errmsg("Invalid Kmer Length!")));  }

    size_t num_kmers = seq_get_num_generable_kmers(seq_get_length(dna, DNA), k);
    size_t data_bytes = seq_get_number_of_bytes_from_length(k, KMER);
      
    if (SRF_IS_FIRSTCALL())
    {
        MemoryContext oldcontext;
        funcctx = SRF_FIRSTCALL_INIT(); // call_cntr is set to 0 + multi_call_memory_ctx is set (which is memory context for any memory that is to be reused across multiple calls of the SRF)

        /* switch to memory context appropriate for multiple function calls */
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
        
        /* total number of tuples to be returned */
        funcctx->max_calls = num_kmers;

        MemoryContextSwitchTo(oldcontext);
    }

      /* stuff done on every call of the function */
    funcctx = SRF_PERCALL_SETUP();

    call_cntr = funcctx->call_cntr;
    max_calls = funcctx->max_calls;


    if (call_cntr < max_calls)    /* do when there is more left to send */
    {
        Datum   result;  // to be returned at the end

        sequence  *kmer = (sequence *) palloc(sizeof(sequence) + data_bytes);
        SET_VARSIZE(kmer, sizeof(sequence) + data_bytes);
        kmer->overflow = seq_get_overflow(k, KMER);
        uint8_t shift = kmer->overflow == 0? 0 :(4 - kmer->overflow) * 2;
        uint8_t mask = 0b11111111 << shift;

        size_t init_byte = (2 * call_cntr) / 8;
        uint8_t init_pos = (2 * call_cntr) % 8;
        uint8_t step = 2 * k - 1;
        for (size_t c = 0; c < data_bytes; ++c) {
          kmer->data[c] = dna->data[c + init_byte];
          kmer->data[c] <<= init_pos;
          if ((init_pos + step) / 8 != c) {
              kmer->data[c] |= dna->data[c + init_byte + 1] >> (8 - init_pos);
          }            
        }

        kmer->data[data_bytes-1] &= mask;

        // elog(NOTICE, "iteration %d", call_cntr);
        // elog(NOTICE, "overflow %d", kmer->overflow);
        // elog(NOTICE, "kmer->data[0]: %s ", seq_get_byte_binary_representation(kmer->data[0]));
        // elog(NOTICE, "kmer string: %s \n", seq_sequence_to_string(kmer,KMER));

        result = PointerGetDatum(kmer);

        SRF_RETURN_NEXT(funcctx, result);

    }
    else   
    {
        SRF_RETURN_DONE(funcctx);
    }

}
