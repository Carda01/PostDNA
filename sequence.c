#include "sequence.h"
#include <stdio.h>
#include <string.h>
#include "funcapi.h"
#include "catalog/pg_type.h"

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
      if(globalQkmerFlag == 2){
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
            errmsg("Invalid DNA base: character number %d is '%c' which is not a valid DNA base", i, seq_str[i])));

        }}
      else {
        ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
          errmsg("Invalid DNA base: character number %d is '%c' which is not a valid DNA base", i, seq_str[i])));}
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
      if(globalQkmerFlag == 2){
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
        }}
      else{
          ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
            errmsg("Decode failed, character number %d is corrupted"), i));}
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




// /* Structure to store function state */
// typedef struct {
//     sequence *seq;        /* Input sequence */
//     uint8_t k;            /* Length of the k-mer */
//     size_t current_index; /* Current index of the k-mer */
//     size_t num_kmers;     /* Total number of k-mers */
//     uint8_t data_bytes;   /* Number of bytes needed for each k-mer */
// } KmerState2;


// PG_FUNCTION_INFO_V1(generate_kmers);
// Datum generate_kmers(PG_FUNCTION_ARGS)
// {
//     globalQkmerFlag = 1;
//     char *str = PG_GETARG_CSTRING(0);
//     uint8_t k = PG_GETARG_INT32(1);

//     sequence* sequence = seq_string_to_sequence(str);

//     const size_t num_kmers = seq_get_num_generable_kmers(strlen(str), k);
//     const uint8_t data_bytes = seq_get_number_of_bytes(k);
      
//     sequence *kmers;
//     FuncCallContext  *funcctx;
    
//     if (SRF_IS_FIRSTCALL())
//     {
//         MemoryContext oldcontext;

//         funcctx = SRF_FIRSTCALL_INIT();
//         oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
//         /* One-time setup code appears here: */

//         kmer = (sequence *) palloc(sizeof(sequence));

//        size_t init_byte = 0;
//        uint8_t init_pos = 0;
//        uint8_t step = 2 * k - 1;
//        for (size_t c = 0; c < data_bytes; ++c) {
//            kmer.data[c] = seq->data[c + init_byte];
//            kmer.data[c] <<= init_pos;
//            if ((init_pos + step) / 8 != c) {
//                kmer.data[c] |= seq->data[c + init_byte + 1] >> (8 - init_pos);
//            }
//        }

//         /* Save the kmer in the function context */
//         funcctx->user_fctx = kmer;
//         funcctx->max_calls = num_kmers;

//         /* Define the result tuple descriptor */
//         TupleDesc tupdesc = CreateTemplateTupleDesc(1);
//         TupleDescInitEntry(tupdesc, (AttrNumber) 1, "Kmer", TEXTOID, -1, 0);
//         funcctx->tuple_desc = BlessTupleDesc(tupdesc);

//         MemoryContextSwitchTo(oldcontext);
//     }

//     /* Retrieve the function call context */
//     funcctx = SRF_PERCALL_SETUP();
//     call_cntr = funcctx->call_cntr;
//     max_calls = funcctx->max_calls;
//     kmer = (sequence *) funcctx->user_fctx;

//     if (call_cntr < max_calls)  
//     {

//         HeapTuple    tuple;
//         Datum        result;


//     }

//     /* No more results */
//     SRF_RETURN_DONE(funcctx);

// }




/* Structure to store function state */
typedef struct {
    sequence *seq;        /* Input sequence */
    uint8_t k;            /* Length of the k-mer */
    size_t current_index; /* Current index of the k-mer */
    size_t num_kmers;     /* Total number of k-mers */
    uint8_t data_bytes;   /* Number of bytes needed for each k-mer */
} KmerState;

/* Function declaration */
PG_FUNCTION_INFO_V1(generate_kmers);
Datum generate_kmers(PG_FUNCTION_ARGS)
{
    FuncCallContext *funcctx;
    KmerState *state;

    /* On the first call, initialize the function context and state */
    if (SRF_IS_FIRSTCALL()) {
        /* Create a function call context for cross-call persistence */
        funcctx = SRF_FIRSTCALL_INIT();

        /* Switch to a memory context appropriate for multiple calls */
        MemoryContext oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

        /* Allocate and initialize function state */
        state = (KmerState *) palloc(sizeof(KmerState));
        funcctx->user_fctx = state;

        /* Get the sequence and k-mer length from function arguments */
        state->seq = (sequence *) PG_GETARG_POINTER(0);
        state->k = PG_GETARG_UINT8(1);

        /* Validate the k-mer length */
        if (state->k > 32) {
            ereport(ERROR,
                    (errmsg("Invalid k size (>32)")));
        }

        /* Calculate the number of k-mers that can be generated */
        state->num_kmers = seq_get_num_generable_kmers(seq_get_length(state->seq), state->k);
        state->data_bytes = seq_get_number_of_bytes(state->k);

        /* Initialize state for generating k-mers */
        state->current_index = 0;

        MemoryContextSwitchTo(oldcontext);
    }

    /* Retrieve the function context and state */
    funcctx = SRF_PERCALL_SETUP();
    state = (KmerState *) funcctx->user_fctx;

    /* Generate the next k-mer */
    if (state->current_index < state->num_kmers) {
        /* Allocate memory for the current k-mer */
        sequence kmer;
        kmer.k = state->k;
        kmer.data = (uint8_t *) palloc(sizeof(uint8_t) * state->data_bytes);

        size_t init_byte = (2 * state->current_index) / 8;
        uint8_t init_pos = (2 * state->current_index) % 8;
        uint8_t step = 2 * state->k - 1;
        for (size_t c = 0; c < state->data_bytes; ++c) {
            kmer.data[c] = state->seq->data[c + init_byte];
            kmer.data[c] <<= init_pos;
            if ((init_pos + step) / 8 != c) {
                kmer.data[c] |= state->seq->data[c + init_byte + 1] >> (8 - init_pos);
            }
        }

        /* Prepare the result tuple */
        Datum values[1];
        bool nulls[1] = {false};
        HeapTuple tuple;
        Datum result;

        values[0] = PointerGetDatum(&kmer);

        /* Create the tuple and return it */
        tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);
        result = HeapTupleGetDatum(tuple);

        /* Return the current k-mer */
        SRF_RETURN_NEXT(funcctx, result);

        /* Update state for the next k-mer */
        state->current_index++;
    }

    /* No more k-mers */
    SRF_RETURN_DONE(funcctx);
}
