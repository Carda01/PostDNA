#ifndef SPGISTKMER_H
#define SPGISTKMER_H

#include "sequence.h"

/*
 * Max prefix length in bytes is 8, as the longest kmer we can have has length 
 * 32. And because every byte can contain up to 4 bases, 32/4 = 8
 */
#define SPGIST_MAX_PREFIX_LENGTH 8

/* Struct for sorting values in picksplit */
typedef struct spgNodePtr
{
	Datum		d;
	int			i;
	int16		c;
} spgNodePtr;


void sequenceCopy(sequence* target, sequence* source, int target_start, int length);
inline size_t kmer_get_length(sequence* seq);
inline uint8_t get_base_at_index(const uint8_t* data, int index);

static Datum formSeqDatum(const uint8_t *data, int begin, int datalen);
static int commonPrefix(const uint8_t *a, const uint8_t *b, int starta, int lena, int lenb);
static bool searchChar(Datum *nodeLabels, int nNodes, int16 c, int *i);

Datum spg_sequence_config(PG_FUNCTION_ARGS);
Datum spg_sequence_choose(PG_FUNCTION_ARGS);
Datum spg_sequence_picksplit(PG_FUNCTION_ARGS);
Datum spg_sequence_inner_consistent(PG_FUNCTION_ARGS);
Datum spg_text_leaf_consistent(PG_FUNCTION_ARGS);

#endif
