#include "spgistkmer.h"
#include "qkmer.h"
#include "kmer.h"

#include "access/spgist.h"
#include "sequence.h"
#include "utils/datum.h"
#include "utils/pg_locale.h"

inline size_t kmer_get_length(sequence* seq) {
    return seq_get_length(seq, KMER);
}

/*
 * Find the length of the common prefix of a and b
 */
static int
common_prefix(const uint8_t *a, const uint8_t *b, int starta, int lena, int lenb)
{
	int i = 0;

	while (i < lena &&
           i < lenb &&
           kmer_get_base_at_index(a, i + starta) == kmer_get_base_at_index(b, i))
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
search_base(Datum *nodeLabels, int nNodes, int16 c, int *i)
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

		commonLen = common_prefix(inData,
								 prefixData,
                                 in->level,
								 inLen - in->level,
								 prefixLen);


		if (commonLen == prefixLen)
		{
			if (inLen - in->level > commonLen)
				nodeBase = kmer_get_base_at_index(inData, in->level + commonLen);
			else {
				nodeBase = -1;
            }
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
					kmer_create_subseq(prefixData, 0, commonLen);
			}
			out->result.splitTuple.prefixNNodes = 1;
			out->result.splitTuple.prefixNodeLabels =
				(Datum *) palloc(sizeof(Datum));
			out->result.splitTuple.prefixNodeLabels[0] =
				Int16GetDatum(kmer_get_base_at_index(prefixData, commonLen));

			out->result.splitTuple.childNodeN = 0;

			if (prefixLen - commonLen == 1)
			{
				out->result.splitTuple.postfixHasPrefix = false;
			}
			else
			{
				out->result.splitTuple.postfixHasPrefix = true;
				out->result.splitTuple.postfixPrefixDatum =
					kmer_create_subseq(prefixData,
                                 commonLen + 1,
								 prefixLen - commonLen - 1);
			}

			PG_RETURN_VOID();
		}
	}
	else if (inLen > in->level)
	{
        nodeBase = kmer_get_base_at_index(inData, in->level);
	}
	else
	{
		nodeBase = -1;
	}

    

	/* Look up nodeBase in the node label array */
	if (search_base(in->nodeLabels, in->nNodes, nodeBase, &i))
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
				kmer_create_subseq(inData,
                             in->level + levelAdd,
							 inLen - in->level - levelAdd);
		else
			out->result.matchNode.restDatum =
				kmer_create_subseq(NULL, 0, 0);
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
    
	spgPickSplitIn *in = (spgPickSplitIn *) PG_GETARG_POINTER(0);
	spgPickSplitOut *out = (spgPickSplitOut *) PG_GETARG_POINTER(1);
	sequence	   *seq0 = DatumGetSEQP(in->datums[0]);
	int			i,
				commonLen;
	spgNodePtr *nodes;

	/* Identify longest common prefix, if any */
    commonLen = kmer_get_length(seq0);

	for (i = 1; i < in->nTuples && commonLen > 0; i++)
	{
		sequence	   *seqi = DatumGetSEQP(in->datums[i]);
		int			tmp = common_prefix(seq0->data,
									   seqi->data,
                                       0,
									   kmer_get_length(seq0),
									   kmer_get_length(seqi));

		if (tmp < commonLen)
			commonLen = tmp;
	}

	/* Set node prefix to be that string, if it's not empty */
	if (commonLen == 0)
	{
		out->hasPrefix = false;
	}
	else
	{
		out->hasPrefix = true;
		out->prefixDatum = kmer_create_subseq(seq0->data, 0, commonLen);
	}

	/* Extract the node label (first non-common byte) from each value */
	nodes = (spgNodePtr *) palloc(sizeof(spgNodePtr) * in->nTuples);

	for (i = 0; i < in->nTuples; i++)
	{
		sequence	   *seqi = DatumGetSEQP(in->datums[i]);

		if (commonLen < kmer_get_length(seqi)){
			nodes[i].c = kmer_get_base_at_index(seqi->data, commonLen);
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
		Datum		leafD;

		if (i == 0 || nodes[i].c != nodes[i - 1].c)
		{
			out->nodeLabels[out->nNodes] = Int16GetDatum(nodes[i].c);
			out->nNodes++;
		}

		if (commonLen < kmer_get_length(seqi))
			leafD = kmer_create_subseq(seqi->data, 
                                 commonLen + 1,
								 kmer_get_length(seqi) - commonLen - 1);
		else
			leafD = kmer_create_subseq(NULL, 0, 0);

		out->leafTupleDatums[nodes[i].i] = leafD;
		out->mapTuplesToNodes[nodes[i].i] = out->nNodes - 1;
	}

	PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(spg_sequence_inner_consistent);
Datum
spg_sequence_inner_consistent(PG_FUNCTION_ARGS)
{

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
	Assert(reconstructedValue == NULL ? in->level == 0 :
		   kmer_get_length(reconstructedValue) == in->level);

	maxReconstrLen = in->level + 1;
	if (in->hasPrefix)
	{
		prefixSeq = DatumGetSEQP(in->prefixDatum);
		prefixLen = kmer_get_length(prefixSeq);
        prefixSize = seq_get_number_of_bytes_from_length(prefixLen, KMER);
		maxReconstrLen += prefixLen;
	}


    reconstrSeq = seq_create_empty_sequence(maxReconstrLen, KMER);

	if (in->level)
        kmer_fill_copy(reconstrSeq->data,
                     reconstructedValue->data,
                     0,
                     in->level);
	if (prefixLen)
		kmer_fill_copy(reconstrSeq->data,
			   prefixSeq->data,
               in->level,
			   prefixLen);
	/* last bit of reconstrSeq will be filled in below */

	/*
	 * Scan the child nodes.  For each one, complete the reconstructed value
	 * and see if it's consistent with the query.  If so, emit an entry into
	 * the output arrays.
	 */
	out->nodeNumbers = (int *) palloc(sizeof(int) * in->nNodes);
	out->levelAdds = (int *) palloc(sizeof(int) * in->nNodes);
	out->reconstructedValues = (Datum *) palloc(sizeof(Datum) * in->nNodes);
	out->nNodes = 0;

    
    int minusOneCheck = 0;
	for (i = 0; i < in->nNodes; i++)
	{
		int16		nodeChar = DatumGetInt16(in->nodeLabels[i]);
		int			thisLen;
        int         thisSize;
		bool		res = true;
		int			j;

		/* If nodeChar is a dummy value, don't include it in data */
		if (nodeChar < 0){
			thisLen = maxReconstrLen - 1;
            int num_bytes = seq_get_number_of_bytes_from_length(thisLen, KMER);
            sequence *tmp = seq_create_sequence(reconstrSeq->data, thisLen, seq_get_number_of_bytes_from_length(thisLen, KMER), KMER);
            pfree(reconstrSeq);
            reconstrSeq = tmp;
        }
		else
		{
            kmer_set_base_at_index(reconstrSeq->data, maxReconstrLen - 1, nodeChar);
			thisLen = maxReconstrLen;
		}

        thisSize = seq_get_number_of_bytes_from_length(thisLen, KMER);

        if(nodeChar == -1 && minusOneCheck != 0) {
            res = minusOneCheck == 1 ? true : false;
        }
        else {
		for (j = 0; j < in->nkeys; j++)
		{
			StrategyNumber strategy = in->scankeys[j].sk_strategy;
			sequence	   *inSeq;
			int			inLen;
			int			r;


			inSeq = DatumGetSEQP(in->scankeys[j].sk_argument);

			switch (strategy)
			{
				case EqualStrategyNumber:
				case PrefixStrategyNumber:
			        inLen = kmer_get_length(inSeq);

                    int minLen = Min(inLen, thisLen);
                    int commonLen = common_prefix(reconstrSeq->data,
                            inSeq->data,
                            0,
                            minLen,
                            minLen);

					if (commonLen != minLen || 
                            (inLen < thisLen && strategy == EqualStrategyNumber))
						res = false;
					break;
                case ContainStrategyNumber:
                    res = qkmer_contains_until(inSeq, reconstrSeq, thisLen);
                    break;
				default:
					elog(ERROR, "unrecognized strategy number: %d",
						 in->scankeys[j].sk_strategy);
					break;
			}

            if(nodeChar == -1){
                minusOneCheck = res ? 1 : 2;
            }
			if (!res)
				break;			/* no need to consider remaining conditions */
		}
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
        sequence       *fullSeq = seq_create_empty_sequence(fullLen, KMER);
		fullValue = fullSeq->data;

		if (level)
            kmer_fill_copy(fullValue, reconstrValue->data, 0, level);
		if (kmer_get_length(leafValue) > 0)
            kmer_fill_copy(fullValue, leafValue->data, level, kmer_get_length(leafValue));
		out->leafValue = PointerGetDatum(fullSeq);
	}

	/* Perform the required comparison(s) */
	res = true;
	for (j = 0; j < in->nkeys; j++)
	{
		StrategyNumber strategy = in->scankeys[j].sk_strategy;
		sequence	   *query = DatumGetSEQP(in->scankeys[j].sk_argument);
        int            queryLen;
		int			r;

		if (strategy == PrefixStrategyNumber)
		{
			/*
			 * if level >= length of query then reconstrValue must begin with
			 * query (prefix) string, so we don't need to check it again.
			 */
            queryLen = kmer_get_length(query);
			res = (level >= queryLen) || (seq_starts_with(out->leafValue, query, KMER));

			if (!res)			/* no need to consider remaining conditions */
				break;

			continue;
		}


		switch (strategy)
		{
			case EqualStrategyNumber:
                res = seq_equals(out->leafValue, query, KMER);
				break;
            case ContainStrategyNumber:
                res = qkmer_contains_internal(query, out->leafValue);
                break;
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
