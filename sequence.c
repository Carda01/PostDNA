#include "sequence.h"
#include <stdio.h>
#include <string.h>
#include "funcapi.h"
#include "catalog/pg_type.h"
#include "funcapi.h"

int globalQkmerFlag = 0;


inline char* seq_get_string_type(int type) {
    return type == DNA ? "DNA" : type == KMER ? "KMER" : "QKMER";
}


sequence* seq_string_to_sequence(const char *seq_str, int type) {
    const size_t seq_length = strlen(seq_str);

    if(type != DNA && seq_length > KMER_MAX_SIZE){
        ereport(ERROR, (errcode(ERRCODE_NAME_TOO_LONG), errmsg("%s length should be less than or equal 32.", seq_get_string_type(type))));
    }

    size_t num_bytes;
    uint8_t* data = seq_encode(seq_str, seq_length, &num_bytes, type);

    sequence *seq = (sequence *) palloc(sizeof(sequence) + num_bytes);
    SET_VARSIZE(seq, sizeof(sequence) + num_bytes);
    seq->overflow = seq_get_overflow(seq_length, type);

    memcpy(seq->data, data, num_bytes);

    free(data);
    return seq;
}

inline int seq_bases_per_byte(int type) {
  return type == QKMER ? 2 : 4;
}

char *seq_sequence_to_string(sequence *seq, int type) {
    return seq_decode(seq->data, seq_get_length(seq, type), type);
}

inline size_t seq_get_number_of_bytes(size_t seq_len, int type) {
  return (seq_len / seq_bases_per_byte(type)) + (seq_get_overflow(seq_len, type) != 0);
}

inline size_t seq_get_length(sequence* seq, int type){
  return (VARSIZE(seq) - sizeof(sequence) - 1) * seq_bases_per_byte(type) + (seq->overflow == 0 ? seq_bases_per_byte(type) : seq->overflow);
}

inline uint8_t seq_get_overflow(size_t seq_length, int type) {
    return seq_length % (seq_bases_per_byte(type));
}

// Helper used in debug, remember to free memory
char* seq_get_byte_binary_representation(const uint8_t value) {
  char* binary_representation = malloc(9);
  for (int i = 7; i >= 0; i--) {
    binary_representation[7-i] = '0' + ((value >> i) & 1);
  }
  binary_representation[8] = '\0';
  return binary_representation;
}


uint8_t *seq_encode(const char *seq_str, const size_t sequence_len, size_t *data_bytes, int type) {
  *data_bytes = seq_get_number_of_bytes(sequence_len, type);
  uint8_t *data = (uint8_t *) malloc(sizeof(uint8_t) * (*data_bytes));
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

  char *sequence = malloc(sizeof(char) * (sequence_len + 1));

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

    for (size_t i = 0; i < seq_get_length(seq1, type); ++i) {
        if (seq1->data[i] != seq2->data[i]) {
            return false;
        }
    }

    return true;
}

// // TODO rework it to generate variable-length kmers
// sequence *seq_generate_kmers(sequence* seq, uint8_t k) {
//    if (k > 32) {
//        printf("Invalid k size (>32)");
//        return NULL;
//    }

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

//    return kmers;
// }



PG_FUNCTION_INFO_V1(generate_kmers);
Datum generate_kmers(PG_FUNCTION_ARGS)
{
    FuncCallContext     *funcctx; // context for all info we need accross calls
    int                  call_cntr;
    int                  max_calls;
    TupleDesc            tupdesc; //////////// one type column not composite soo?
    AttInMetadata        *attinmeta; //////////// 
    sequence             *kmer;
    // HeapTuple            tuple;
    // bool *isnull;
    // Datum  dat[1] ;


    sequence *dna = PG_GETARG_SEQ_P(0); //the dna sequence
    uint8_t k = PG_GETARG_INT32(1); // k (for generating k-mers)
    size_t num_kmers = seq_get_num_generable_kmers(seq_get_length(dna, DNA), k);
    uint8_t data_bytes = seq_get_number_of_bytes(k, KMER);
      
    if (SRF_IS_FIRSTCALL())
    {
        MemoryContext oldcontext;
        funcctx = SRF_FIRSTCALL_INIT(); // call_cntr is set to 0 + multi_call_memory_ctx is set (which is memory context for any memory that is to be reused across multiple calls of the SRF)

        /* switch to memory context appropriate for multiple function calls */
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
        
        /* total number of tuples to be returned */
        funcctx->max_calls = num_kmers;

        /* Build a tuple descriptor for our result type */
        if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
            ereport(ERROR,
                    (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                     errmsg("function returning record called in context "
                            "that cannot accept type record")));

        // tupdesc = BlessTupleDesc(tupdesc);
        attinmeta = TupleDescGetAttInMetadata(tupdesc); ////////////
        funcctx->attinmeta = attinmeta; ////////////

        MemoryContextSwitchTo(oldcontext);
    }

      /* stuff done on every call of the function */
    funcctx = SRF_PERCALL_SETUP();

    call_cntr = funcctx->call_cntr;
    max_calls = funcctx->max_calls;
    attinmeta = funcctx->attinmeta; //////////

    if (call_cntr < max_calls)    /* do when there is more left to send */
    {
        char       **values;
        char       *temp;
        HeapTuple    tuple;
        Datum   result;  // to be returned at the end

        kmer = (sequence *) palloc(sizeof(sequence) + data_bytes);

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


         values = (char **) palloc(1 * sizeof(char *));
         values[0] = (char *) palloc((k+1) * sizeof(char));

         snprintf(values[0], k+1, "%s", seq_sequence_to_string(kmer, KMER));
        /* build a tuple */
        tuple = BuildTupleFromCStrings(attinmeta, values);
        
        //dat[0] = PointerGetDatum(kmer);
        //dat[0] = PointerGetDatum(seq_sequence_to_string(kmer));
        //tuple = heap_form_tuple(tupdesc, dat, isnull);

        // if (isnull)
        //     ereport(ERROR,
        //             (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
        //              errmsg("Tuple is zero")));

        /* make the tuple into a datum */
        result = HeapTupleGetDatum(tuple);

        /* clean up (this is not really necessary) */
        // pfree(values[0]);
        // pfree(values);
        pfree(kmer);

        SRF_RETURN_NEXT(funcctx, result);


    }
    else    /* do when there is no more left */
    {
        SRF_RETURN_DONE(funcctx);
    }


}



