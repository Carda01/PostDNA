#include "spgistkmer.h"

// TODO: see which includes can be removed

#include "access/spgist.h"
#include "catalog/pg_type.h"
#include "common/int.h"
#include "mb/pg_wchar.h"
#include "sequence.h"
#include "postgres.h"
#include "utils/datum.h"
#include "utils/fmgrprotos.h"
#include "utils/pg_locale.h"
#include "utils/varlena.h"


void logSeq(sequence *seq) {
    if(seq == NULL){
        elog(DEBUG1, "pointer is NULL");
    }
    else {
        elog(DEBUG1, "sequence: %s", seq_sequence_to_string(seq, KMER));
    }
}


void sequenceCopy(uint8_t* target, uint8_t* source, int target_start, int length) {
    int data_bytes = length / 4;
    int overflow = length % 4;
    int start_byte = target_start / 4;
    int start_overflow = target_start % 4;
    int bits_copied = 0;
    for (int i = 0; bits_copied < length; i++) {
        target[start_byte + i] |= source[i] >> (start_overflow * 2);
        bits_copied += 4 - start_overflow;
        if(bits_copied < length) {
            target[start_byte + i + 1] |= source[i] << ((4 - start_overflow) * 2);
            bits_copied += start_overflow;
        }
    }
    
    int final_length = target_start + 1 + length;
    int final_size = seq_get_number_of_bytes_from_length(final_length, KMER);
    if ((final_length) % 4){ 
        uint8_t cleaner = 0x0;
        uint8_t i = 8 - 2 * (final_length % 4);
        while(i <= 6) {
            cleaner |= (BASE_MASK << i); 
            i+=2;
        }
        target[final_size - 1] &= cleaner;
    }
}

void set_base_at_index(uint8_t* data, int index, uint8_t nodeBase) {
  int byte_index = index / 4;
  int overflow = index % 4;
  uint8_t shift = (6 - 2 * overflow);
  uint8_t shifted_element = nodeBase << shift;
  uint8_t cleaner = ~(BASE_MASK << shift);
  data[byte_index] = (data[byte_index] & cleaner) | shifted_element;
}

inline size_t kmer_get_length(sequence* seq) {
    return seq_get_length(seq, KMER);
}

/*
 * Get the base from the data structure, given an index that would be the
 * same that element would have if it was a string
 */
inline uint8_t get_base_at_index(const uint8_t* data, int index){
  int byte_index, overflow;
  byte_index = index / 4;
  overflow = index % 4;
  uint8_t shift = 6 - 2 * overflow;
  return (data[byte_index] >> shift) & BASE_MASK;
}


/*
 * Form a sequence datum from the given bytes
 */
static Datum
formSeqDatum(const uint8_t *data, int begin, int datalen)
{
    sequence* seq;
    int num_bytes = seq_get_number_of_bytes_from_length(datalen, KMER);
    uint8_t overflow = begin % 4;
    data += (begin / 4);
    if(overflow == 0){
        seq = seq_create_sequence(data, datalen, num_bytes, KMER);
    }
    else {
        uint8_t *new_data = (uint8_t *) palloc0(sizeof(uint8_t) * (num_bytes));
        int bits_copied = 0;
        for(int i = 0; bits_copied < datalen; i++){
            new_data[i] = data[i] << (overflow * 2);
            bits_copied += (4 - overflow);
            if(bits_copied < datalen){
                new_data[i] |= data[i + 1] >> ((4 - overflow)*2);
                bits_copied += overflow;
            }
        }
        seq = seq_create_sequence(new_data, datalen, num_bytes, KMER);
        pfree(new_data);
    }

    if (datalen % 4){ 
        uint8_t cleaner = 0x0;
        uint8_t i = 8 - 2 * (datalen % 4);
        while(i <= 6) {
            cleaner |= (BASE_MASK << i); 
            i+=2;
        }
        seq->data[num_bytes-1] &= cleaner;
    }

	return PointerGetDatum(seq);
}

/*
 * Find the length of the common prefix of a and b
 */
static int
commonPrefix(const uint8_t *a, const uint8_t *b, int starta, int lena, int lenb)
{
	int			i = 0;

	while (i < lena && i < lenb && get_base_at_index(a, i + starta) == get_base_at_index(b, i))
	{
		i++;
	}

	return i;
}

/*
 * Linear search an array of int16 datums for a match to c
 *
 * On success, *i gets the match location; on failure, it gets where to insert
 */
static bool
searchBase(Datum *nodeLabels, int nNodes, int16 c, int *i)
{
    *i = 0;
    while(*i < nNodes) {
        if(c == nodeLabels[*i]) {
            return true;
        }
        *i += 1;
    }
	return false;
}

PG_FUNCTION_INFO_V1(spg_sequence_config);
Datum
spg_sequence_config(PG_FUNCTION_ARGS)
{
    elog(DEBUG1, "config");
	spgConfigIn *cfgin = (spgConfigIn *) PG_GETARG_POINTER(0);
	spgConfigOut *cfg = (spgConfigOut *) PG_GETARG_POINTER(1);

	cfg->prefixType = cfgin->attType;
	cfg->labelType = INT2OID;
	cfg->canReturnData = true;
	cfg->longValuesOK = false;	/* max len is 32 */
	PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(spg_sequence_choose);
Datum
spg_sequence_choose(PG_FUNCTION_ARGS)
{
    //elog(DEBUG1, "choose");
	spgChooseIn *in = (spgChooseIn *) PG_GETARG_POINTER(0);
	spgChooseOut *out = (spgChooseOut *) PG_GETARG_POINTER(1);
	sequence	   *inSeq = DatumGetSEQP(in->datum);
	uint8_t	   *inData = inSeq->data;
	int			inLen = kmer_get_length(inSeq);
	uint8_t	   *prefixData = NULL;
	int			prefixLen = 0;
	int			commonLen = 0;
	int16		nodeBase = 0;
	int			i = 0;

	/* Check for prefix match, set nodeBase to first byte after prefix */
	if (in->hasPrefix)
	{
		sequence	   *prefixSeq = DatumGetSEQP(in->prefixDatum);

		prefixData = prefixSeq->data;
		prefixLen = kmer_get_length(prefixSeq);

		commonLen = commonPrefix(inData,
								 prefixData,
                                 in->level,
								 inLen - in->level,
								 prefixLen);

		if (commonLen == prefixLen)
		{
			if (inLen - in->level > commonLen)
				nodeBase = get_base_at_index(inData, in->level + commonLen);
			else
				nodeBase = -1;
		}
		else
		{
			/* Must split tuple because incoming value doesn't match prefix */
			out->resultType = spgSplitTuple;

			if (commonLen == 0)
			{
				out->result.splitTuple.prefixHasPrefix = false;
			}
			else
			{
				out->result.splitTuple.prefixHasPrefix = true;
				out->result.splitTuple.prefixPrefixDatum =
					formSeqDatum(prefixData, 0, commonLen);
			}
			out->result.splitTuple.prefixNNodes = 1;
			out->result.splitTuple.prefixNodeLabels =
				(Datum *) palloc(sizeof(Datum));
			out->result.splitTuple.prefixNodeLabels[0] =
				Int16GetDatum(get_base_at_index(prefixData, commonLen));

			out->result.splitTuple.childNodeN = 0;

			if (prefixLen - commonLen == 1)
			{
				out->result.splitTuple.postfixHasPrefix = false;
			}
			else
			{
				out->result.splitTuple.postfixHasPrefix = true;
				out->result.splitTuple.postfixPrefixDatum =
					formSeqDatum(prefixData,
                                 commonLen + 1,
								 prefixLen - commonLen - 1);
			}

			PG_RETURN_VOID();
		}
	}
	else if (inLen > in->level)
	{
        nodeBase = get_base_at_index(inData, in->level);
	}
	else
	{
		nodeBase = -1;
	}

	/* Look up nodeBase in the node label array */
	if (searchBase(in->nodeLabels, in->nNodes, nodeBase, &i))
	{
		/*
		 * Descend to existing node.  (If in->allTheSame, the core code will
		 * ignore our nodeN specification here, but that's OK.  We still have
		 * to provide the correct levelAdd and restDatum values, and those are
		 * the same regardless of which node gets chosen by core.)
		 */
		int			levelAdd;

		out->resultType = spgMatchNode;
		out->result.matchNode.nodeN = i;
		levelAdd = commonLen;
		if (nodeBase >= 0)
			levelAdd++;
		out->result.matchNode.levelAdd = levelAdd;
		if (inLen - in->level - levelAdd > 0)
			out->result.matchNode.restDatum =
				formSeqDatum(inData,
                             in->level + levelAdd,
							 inLen - in->level - levelAdd);
		else
			out->result.matchNode.restDatum =
				formSeqDatum(NULL, 0, 0); // TODO, check if NULL works

    }
    else if (in->allTheSame)
    {
        /*
         * Can't use AddNode action, so split the tuple.  The upper tuple has
         * the same prefix as before and uses a dummy node label -2 for the
         * lower tuple.  The lower tuple has no prefix and the same node
         * labels as the original tuple.
         *
         * Note: it might seem tempting to shorten the upper tuple's prefix,
         * if it has one, then use its last byte as label for the lower tuple.
         * But that doesn't win since we know the incoming value matches the
         * whole prefix: we'd just end up splitting the lower tuple again.
         */
        out->resultType = spgSplitTuple;
        out->result.splitTuple.prefixHasPrefix = in->hasPrefix;
        out->result.splitTuple.prefixPrefixDatum = in->prefixDatum;
        out->result.splitTuple.prefixNNodes = 1;
        out->result.splitTuple.prefixNodeLabels = (Datum *) palloc(sizeof(Datum));
        out->result.splitTuple.prefixNodeLabels[0] = Int16GetDatum(-2);
        out->result.splitTuple.childNodeN = 0;
        out->result.splitTuple.postfixHasPrefix = false;
    }
    else
    {
        /* Add a node for the not-previously-seen nodeBase value */
        out->resultType = spgAddNode;
        out->result.addNode.nodeLabel = Int16GetDatum(nodeBase);
        out->result.addNode.nodeN = i;
    }

    PG_RETURN_VOID();
}


int compare_int(int a, int b){
    if(a == b){
        return 0;
    }
    else if(a > b){
        return 1;
    }
    else 
        return -1;
}

/* qsort comparator to sort spgNodePtr structs by "c" */
static int
cmpNodePtr(const void *a, const void *b)
{
	const spgNodePtr *aa = (const spgNodePtr *) a;
	const spgNodePtr *bb = (const spgNodePtr *) b;

	return compare_int(aa->c, bb->c);
}


PG_FUNCTION_INFO_V1(spg_sequence_picksplit);
Datum
spg_sequence_picksplit(PG_FUNCTION_ARGS)
{
    elog(DEBUG1, "picksplit");
	spgPickSplitIn *in = (spgPickSplitIn *) PG_GETARG_POINTER(0);
	spgPickSplitOut *out = (spgPickSplitOut *) PG_GETARG_POINTER(1);
	sequence	   *seq0 = DatumGetSEQP(in->datums[0]);
	int			i,
				commonLen;
	spgNodePtr *nodes;

	/* Identify longest common prefix, if any */
    commonLen = kmer_get_length(seq0);
    elog(DEBUG1, "commonLen: %d", commonLen);

	for (i = 1; i < in->nTuples && commonLen > 0; i++)
	{
		sequence	   *seqi = DatumGetSEQP(in->datums[i]);
		int			tmp = commonPrefix(seq0->data,
									   seqi->data,
                                       0,
									   kmer_get_length(seq0),
									   kmer_get_length(seqi));

		if (tmp < commonLen)
			commonLen = tmp;
	}
    elog(DEBUG1, "commonLen: %d", commonLen);

	/* Set node prefix to be that string, if it's not empty */
	if (commonLen == 0)
	{
		out->hasPrefix = false;
	}
	else
	{
		out->hasPrefix = true;
		out->prefixDatum = formSeqDatum(seq0->data, 0, commonLen);
	}

	/* Extract the node label (first non-common byte) from each value */
	nodes = (spgNodePtr *) palloc(sizeof(spgNodePtr) * in->nTuples);

	for (i = 0; i < in->nTuples; i++)
	{
		sequence	   *seqi = DatumGetSEQP(in->datums[i]);

		if (commonLen < kmer_get_length(seqi)){
			nodes[i].c = get_base_at_index(seqi->data, commonLen);
        }
		else{
			nodes[i].c = -1;	/* use -1 if string is all common */
        }
		nodes[i].i = i;
		nodes[i].d = in->datums[i];
	}

	qsort(nodes, in->nTuples, sizeof(*nodes), cmpNodePtr);

	/* And emit results */
	out->nNodes = 0;
	out->nodeLabels = (Datum *) palloc(sizeof(Datum) * in->nTuples);
	out->mapTuplesToNodes = (int *) palloc(sizeof(int) * in->nTuples);
	out->leafTupleDatums = (Datum *) palloc(sizeof(Datum) * in->nTuples);

	for (i = 0; i < in->nTuples; i++)
	{
		sequence	   *seqi = DatumGetSEQP(nodes[i].d);
        // logSeq(nodes[i].d);
		Datum		leafD;

		if (i == 0 || nodes[i].c != nodes[i - 1].c)
		{
			out->nodeLabels[out->nNodes] = Int16GetDatum(nodes[i].c);
			out->nNodes++;
		}

		if (commonLen < kmer_get_length(seqi))
			leafD = formSeqDatum(seqi->data, 
                                 commonLen + 1,
								 kmer_get_length(seqi) - commonLen - 1);
		else
			leafD = formSeqDatum(NULL, 0, 0); // TODO: check if it works when passing null

        // logSeq(leafD);
        // elog(DEBUG1, "nNodes: %d", out->nNodes);
		out->leafTupleDatums[nodes[i].i] = leafD;
		out->mapTuplesToNodes[nodes[i].i] = out->nNodes - 1;
	}

	PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(spg_sequence_inner_consistent);
Datum
spg_sequence_inner_consistent(PG_FUNCTION_ARGS)
{
    elog(DEBUG1, "inner_consistent");
	spgInnerConsistentIn *in = (spgInnerConsistentIn *) PG_GETARG_POINTER(0);
	spgInnerConsistentOut *out = (spgInnerConsistentOut *) PG_GETARG_POINTER(1);
	sequence	   *reconstructedValue;
	sequence	   *reconstrSeq;
	int			maxReconstrLen;
	sequence	   *prefixSeq = NULL;
	int			prefixLen = 0;
    int         prefixSize = 0;
	int			i;

	/*
	 * Reconstruct values represented at this tuple, including parent data,
	 * prefix of this tuple if any, and the node label if it's non-dummy.
	 * in->level should be the length of the previously reconstructed value,
	 * and the number of bytes added here is prefixLen or prefixLen + 1.
	 *
	 * Note: we assume that in->reconstructedValue isn't toasted and doesn't
	 * have a short varlena header.  This is okay because it must have been
	 * created by a previous invocation of this routine, and we always emit
	 * long-format reconstructed values.
	 */

	reconstructedValue = DatumGetSEQP(in->reconstructedValue);
    logSeq(reconstructedValue);
	Assert(reconstructedValue == NULL ? in->level == 0 :
		   kmer_get_length(reconstructedValue) == in->level);

	maxReconstrLen = in->level + 1;
    elog(DEBUG1, "maxReconstrLen: %d", maxReconstrLen);
    elog(DEBUG1, "hasPrefix: %d", in->hasPrefix);
	if (in->hasPrefix)
	{
		prefixSeq = DatumGetSEQP(in->prefixDatum);
		prefixLen = kmer_get_length(prefixSeq);
        prefixSize = seq_get_number_of_bytes_from_length(prefixLen, KMER);
		maxReconstrLen += prefixLen;
	}

	reconstrSeq = palloc0(sizeof(sequence) + prefixSize);
	SET_VARSIZE(reconstrSeq, sizeof(sequence) + prefixSize);
    reconstrSeq->overflow = seq_get_overflow(prefixLen, KMER);

	if (in->level)
        sequenceCopy(reconstrSeq->data,
                     reconstructedValue->data,
                     0,
                     in->level);
	if (prefixLen)
		sequenceCopy(reconstrSeq->data,
			   prefixSeq->data,
               in->level,
			   prefixLen);
	/* last byte of reconstrSeq will be filled in below */

	/*
	 * Scan the child nodes.  For each one, complete the reconstructed value
	 * and see if it's consistent with the query.  If so, emit an entry into
	 * the output arrays.
	 */
	out->nodeNumbers = (int *) palloc(sizeof(int) * in->nNodes);
	out->levelAdds = (int *) palloc(sizeof(int) * in->nNodes);
	out->reconstructedValues = (Datum *) palloc(sizeof(Datum) * in->nNodes);
	out->nNodes = 0;

    
    elog(DEBUG1, "in->nNodes: %d", in->nNodes);
	for (i = 0; i < in->nNodes; i++)
	{
		int16		nodeChar = DatumGetInt16(in->nodeLabels[i]);
        elog(DEBUG1, "nodeChar: %d", nodeChar);
		int			thisLen;
        int         thisSize;
		bool		res = true;
		int			j;

		/* If nodeChar is a dummy value, don't include it in data */
		if (nodeChar < 0)
			thisLen = maxReconstrLen - 1;
		else
		{
            set_base_at_index(reconstrSeq->data, maxReconstrLen - 1, nodeChar);
			thisLen = maxReconstrLen;
		}

        thisSize = seq_get_number_of_bytes_from_length(thisLen, KMER);

		for (j = 0; j < in->nkeys; j++)
		{
			StrategyNumber strategy = in->scankeys[j].sk_strategy;
			sequence	   *inSeq;
			int			inLen;
			int			r;


			inSeq = DatumGetSEQP(in->scankeys[j].sk_argument);
			inLen = kmer_get_length(inSeq);

            int commonLen = commonPrefix(reconstrSeq->data, inSeq->data, 0, inLen, thisLen);
            int minLen = Min(inLen, thisLen);

			switch (strategy)
			{
				case EqualStrategyNumber:
					if (commonLen != minLen - 1 || inLen < thisLen)
						res = false;
					break;
				case PrefixStrategyNumber:
					if (commonLen != minLen - 1)
						res = false;
					break;
                // case ContainStrategyNumber:
                //     break;
				default:
					elog(ERROR, "unrecognized strategy number: %d",
						 in->scankeys[j].sk_strategy);
					break;
			}

			if (!res)
				break;			/* no need to consider remaining conditions */
		}

		if (res)
		{
			out->nodeNumbers[out->nNodes] = i;
			out->levelAdds[out->nNodes] = thisLen - in->level;
			SET_VARSIZE(reconstrSeq, sizeof(sequence) + thisSize);
			out->reconstructedValues[out->nNodes] =
				datumCopy(PointerGetDatum(reconstrSeq), false, -1);
			out->nNodes++;
		}
	}

	PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(spg_sequence_leaf_consistent);
Datum
spg_sequence_leaf_consistent(PG_FUNCTION_ARGS)
{
    // elog(DEBUG1, "leaf_consistent");
	spgLeafConsistentIn *in = (spgLeafConsistentIn *) PG_GETARG_POINTER(0);
	spgLeafConsistentOut *out = (spgLeafConsistentOut *) PG_GETARG_POINTER(1);
	int			level = in->level;
	sequence	   *leafValue,
			   *reconstrValue = NULL;
	uint8_t	   *fullValue;
	int			fullLen;
    int         fullSize;
	bool		res;
	int			j;

	/* all tests are exact */
	out->recheck = false;

	leafValue = DatumGetSEQP(in->leafDatum);

	/* As above, in->reconstructedValue isn't toasted or short. */
	if (DatumGetPointer(in->reconstructedValue))
		reconstrValue = (sequence *) DatumGetPointer(in->reconstructedValue);

	Assert(reconstrValue == NULL ? level == 0 :
		   kmer_get_length(reconstrValue) == level);

	/* Reconstruct the full string represented by this leaf tuple */
	fullLen = level + kmer_get_length(leafValue);
    fullSize = seq_get_number_of_bytes_from_length(fullLen, KMER);
	if (kmer_get_length(leafValue) == 0 && level > 0)
	{
		fullValue = reconstrValue->data;
		out->leafValue = PointerGetDatum(reconstrValue);
	}
	else
	{
		sequence	   *fullSeq = palloc(VARHDRSZ + fullLen);

		SET_VARSIZE(fullSeq, VARHDRSZ + fullSize);
		fullValue = fullSeq->data;
		if (level)
            sequenceCopy(fullValue, reconstrValue->data, 0, level);
		if (kmer_get_length(leafValue) > 0)
            sequenceCopy(fullValue, leafValue->data, level, kmer_get_length(leafValue));
		out->leafValue = PointerGetDatum(fullSeq);
	}

	/* Perform the required comparison(s) */
	res = true;
	for (j = 0; j < in->nkeys; j++)
	{
		StrategyNumber strategy = in->scankeys[j].sk_strategy;
		sequence	   *query = DatumGetSEQP(in->scankeys[j].sk_argument);
		int			queryLen = kmer_get_length(query);
		int			r;

		if (strategy == PrefixStrategyNumber)
		{
			/*
			 * if level >= length of query then reconstrValue must begin with
			 * query (prefix) string, so we don't need to check it again.
			 */
            // Wait for starts_with function
			// res = (level >= queryLen) ||
			// 	DatumGetBool(DirectFunctionCall2Coll(starts_with,
			// 										 out->leafValue,
			// 										 PointerGetDatum(query)));

			if (!res)			/* no need to consider remaining conditions */
				break;

			continue;
		}

        int commonIndex = commonPrefix(fullValue, query->data, 0, fullLen, queryLen);

		switch (strategy)
		{
			case EqualStrategyNumber:
				res = ((commonIndex == fullLen - 1) && (commonIndex == queryLen - 1));
				break;
            // case ContainStrategyNumber:
            //     break;
			default:
				elog(ERROR, "unrecognized strategy number: %d",
					 in->scankeys[j].sk_strategy);
				res = false;
				break;
		}

		if (!res)
			break;				/* no need to consider remaining conditions */
	}

	PG_RETURN_BOOL(res);
}
