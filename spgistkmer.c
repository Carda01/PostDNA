#include "sequence.h"

// TODO: see which includes can be removed

#include "access/spgist.h"
#include "catalog/pg_type.h"
#include "common/int.h"
#include "mb/pg_wchar.h"
#include "utils/datum.h"
#include "utils/fmgrprotos.h"
#include "utils/pg_locale.h"
#include "utils/varlena.h"


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
    size_t num_bytes = seq_get_number_of_bytes_from_length(datalen, KMER);
    int overflow = begin % 4;
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
searchChar(Datum *nodeLabels, int nNodes, int16 c, int *i)
{
    i = 0;
    while(*i < nNodes) {
        if(c == nodeLabels[*i]) {
            return true;
        }
        *i += 1;
    }
	return false;
}

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
	if (searchChar(in->nodeLabels, in->nNodes, nodeBase, &i))
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


Datum
spg_text_picksplit(PG_FUNCTION_ARGS)
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
		int			tmp = commonPrefix(seq0->data,
									   seqi->data,
                                       0,
									   kmer_get_length(seq0),
									   kmer_get_length(seqi));

		if (tmp < commonLen)
			commonLen = tmp;
	}

	/*
	 * Limit the prefix length, if necessary, to ensure that the resulting
	 * inner tuple will fit on a page.
	 */
	commonLen = Min(commonLen, SPGIST_MAX_PREFIX_LENGTH);

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

		if (commonLen < kmer_get_length(seqi))
			nodes[i].c = get_base_at_index(seqi->data, commonLen);
		else
			nodes[i].c = -1;	/* use -1 if string is all common */
		nodes[i].i = i;
		nodes[i].d = in->datums[i];
	}

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
			leafD = formSeqDatum(seqi->data, commonLen + 1,
								  kmer_get_length(seqi) - commonLen - 1);
		else
			leafD = formSeqDatum(NULL, 0, 0); // TODO: check if it works when passing null

		out->leafTupleDatums[nodes[i].i] = leafD;
		out->mapTuplesToNodes[nodes[i].i] = out->nNodes - 1;
	}

	PG_RETURN_VOID();
}

// Datum
// spg_text_inner_consistent(PG_FUNCTION_ARGS)
// {
// 	spgInnerConsistentIn *in = (spgInnerConsistentIn *) PG_GETARG_POINTER(0);
// 	spgInnerConsistentOut *out = (spgInnerConsistentOut *) PG_GETARG_POINTER(1);
// 	bool		collate_is_c = pg_newlocale_from_collation(PG_GET_COLLATION())->collate_is_c;
// 	text	   *reconstructedValue;
// 	text	   *reconstrText;
// 	int			maxReconstrLen;
// 	text	   *prefixText = NULL;
// 	int			prefixSize = 0;
// 	int			i;
// 
// 	/*
// 	 * Reconstruct values represented at this tuple, including parent data,
// 	 * prefix of this tuple if any, and the node label if it's non-dummy.
// 	 * in->level should be the length of the previously reconstructed value,
// 	 * and the number of bytes added here is prefixSize or prefixSize + 1.
// 	 *
// 	 * Note: we assume that in->reconstructedValue isn't toasted and doesn't
// 	 * have a short varlena header.  This is okay because it must have been
// 	 * created by a previous invocation of this routine, and we always emit
// 	 * long-format reconstructed values.
// 	 */
// 	reconstructedValue = (text *) DatumGetPointer(in->reconstructedValue);
// 	Assert(reconstructedValue == NULL ? in->level == 0 :
// 		   VARSIZE_ANY_EXHDR(reconstructedValue) == in->level);
// 
// 	maxReconstrLen = in->level + 1;
// 	if (in->hasPrefix)
// 	{
// 		prefixText = DatumGetTextPP(in->prefixDatum);
// 		prefixSize = VARSIZE_ANY_EXHDR(prefixText);
// 		maxReconstrLen += prefixSize;
// 	}
// 
// 	reconstrText = palloc(VARHDRSZ + maxReconstrLen);
// 	SET_VARSIZE(reconstrText, VARHDRSZ + maxReconstrLen);
// 
// 	if (in->level)
// 		memcpy(VARDATA(reconstrText),
// 			   VARDATA(reconstructedValue),
// 			   in->level);
// 	if (prefixSize)
// 		memcpy(((char *) VARDATA(reconstrText)) + in->level,
// 			   VARDATA_ANY(prefixText),
// 			   prefixSize);
// 	/* last byte of reconstrText will be filled in below */
// 
// 	/*
// 	 * Scan the child nodes.  For each one, complete the reconstructed value
// 	 * and see if it's consistent with the query.  If so, emit an entry into
// 	 * the output arrays.
// 	 */
// 	out->nodeNumbers = (int *) palloc(sizeof(int) * in->nNodes);
// 	out->levelAdds = (int *) palloc(sizeof(int) * in->nNodes);
// 	out->reconstructedValues = (Datum *) palloc(sizeof(Datum) * in->nNodes);
// 	out->nNodes = 0;
// 
// 	for (i = 0; i < in->nNodes; i++)
// 	{
// 		int16		nodeChar = DatumGetInt16(in->nodeLabels[i]);
// 		int			thisLen;
// 		bool		res = true;
// 		int			j;
// 
// 		/* If nodeChar is a dummy value, don't include it in data */
// 		if (nodeChar <= 0)
// 			thisLen = maxReconstrLen - 1;
// 		else
// 		{
// 			((unsigned char *) VARDATA(reconstrText))[maxReconstrLen - 1] = nodeChar;
// 			thisLen = maxReconstrLen;
// 		}
// 
// 		for (j = 0; j < in->nkeys; j++)
// 		{
// 			StrategyNumber strategy = in->scankeys[j].sk_strategy;
// 			text	   *inText;
// 			int			inSize;
// 			int			r;
// 
// 			/*
// 			 * If it's a collation-aware operator, but the collation is C, we
// 			 * can treat it as non-collation-aware.  With non-C collation we
// 			 * need to traverse whole tree :-( so there's no point in making
// 			 * any check here.  (Note also that our reconstructed value may
// 			 * well end with a partial multibyte character, so that applying
// 			 * any encoding-sensitive test to it would be risky anyhow.)
// 			 */
// 			if (SPG_IS_COLLATION_AWARE_STRATEGY(strategy))
// 			{
// 				if (collate_is_c)
// 					strategy -= SPG_STRATEGY_ADDITION;
// 				else
// 					continue;
// 			}
// 
// 			inText = DatumGetTextPP(in->scankeys[j].sk_argument);
// 			inSize = VARSIZE_ANY_EXHDR(inText);
// 
// 			r = memcmp(VARDATA(reconstrText), VARDATA_ANY(inText),
// 					   Min(inSize, thisLen));
// 
// 			switch (strategy)
// 			{
// 				case BTLessStrategyNumber:
// 				case BTLessEqualStrategyNumber:
// 					if (r > 0)
// 						res = false;
// 					break;
// 				case BTEqualStrategyNumber:
// 					if (r != 0 || inSize < thisLen)
// 						res = false;
// 					break;
// 				case BTGreaterEqualStrategyNumber:
// 				case BTGreaterStrategyNumber:
// 					if (r < 0)
// 						res = false;
// 					break;
// 				case RTPrefixStrategyNumber:
// 					if (r != 0)
// 						res = false;
// 					break;
// 				default:
// 					elog(ERROR, "unrecognized strategy number: %d",
// 						 in->scankeys[j].sk_strategy);
// 					break;
// 			}
// 
// 			if (!res)
// 				break;			/* no need to consider remaining conditions */
// 		}
// 
// 		if (res)
// 		{
// 			out->nodeNumbers[out->nNodes] = i;
// 			out->levelAdds[out->nNodes] = thisLen - in->level;
// 			SET_VARSIZE(reconstrText, VARHDRSZ + thisLen);
// 			out->reconstructedValues[out->nNodes] =
// 				datumCopy(PointerGetDatum(reconstrText), false, -1);
// 			out->nNodes++;
// 		}
// 	}
// 
// 	PG_RETURN_VOID();
// }

// Datum
// spg_text_leaf_consistent(PG_FUNCTION_ARGS)
// {
// 	spgLeafConsistentIn *in = (spgLeafConsistentIn *) PG_GETARG_POINTER(0);
// 	spgLeafConsistentOut *out = (spgLeafConsistentOut *) PG_GETARG_POINTER(1);
// 	int			level = in->level;
// 	text	   *leafValue,
// 			   *reconstrValue = NULL;
// 	char	   *fullValue;
// 	int			fullLen;
// 	bool		res;
// 	int			j;
// 
// 	/* all tests are exact */
// 	out->recheck = false;
// 
// 	leafValue = DatumGetTextPP(in->leafDatum);
// 
// 	/* As above, in->reconstructedValue isn't toasted or short. */
// 	if (DatumGetPointer(in->reconstructedValue))
// 		reconstrValue = (text *) DatumGetPointer(in->reconstructedValue);
// 
// 	Assert(reconstrValue == NULL ? level == 0 :
// 		   VARSIZE_ANY_EXHDR(reconstrValue) == level);
// 
// 	/* Reconstruct the full string represented by this leaf tuple */
// 	fullLen = level + VARSIZE_ANY_EXHDR(leafValue);
// 	if (VARSIZE_ANY_EXHDR(leafValue) == 0 && level > 0)
// 	{
// 		fullValue = VARDATA(reconstrValue);
// 		out->leafValue = PointerGetDatum(reconstrValue);
// 	}
// 	else
// 	{
// 		text	   *fullText = palloc(VARHDRSZ + fullLen);
// 
// 		SET_VARSIZE(fullText, VARHDRSZ + fullLen);
// 		fullValue = VARDATA(fullText);
// 		if (level)
// 			memcpy(fullValue, VARDATA(reconstrValue), level);
// 		if (VARSIZE_ANY_EXHDR(leafValue) > 0)
// 			memcpy(fullValue + level, VARDATA_ANY(leafValue),
// 				   VARSIZE_ANY_EXHDR(leafValue));
// 		out->leafValue = PointerGetDatum(fullText);
// 	}
// 
// 	/* Perform the required comparison(s) */
// 	res = true;
// 	for (j = 0; j < in->nkeys; j++)
// 	{
// 		StrategyNumber strategy = in->scankeys[j].sk_strategy;
// 		text	   *query = DatumGetTextPP(in->scankeys[j].sk_argument);
// 		int			queryLen = VARSIZE_ANY_EXHDR(query);
// 		int			r;
// 
// 		if (strategy == RTPrefixStrategyNumber)
// 		{
// 			/*
// 			 * if level >= length of query then reconstrValue must begin with
// 			 * query (prefix) string, so we don't need to check it again.
// 			 */
// 			res = (level >= queryLen) ||
// 				DatumGetBool(DirectFunctionCall2Coll(text_starts_with,
// 													 PG_GET_COLLATION(),
// 													 out->leafValue,
// 													 PointerGetDatum(query)));
// 
// 			if (!res)			/* no need to consider remaining conditions */
// 				break;
// 
// 			continue;
// 		}
// 
// 		if (SPG_IS_COLLATION_AWARE_STRATEGY(strategy))
// 		{
// 			/* Collation-aware comparison */
// 			strategy -= SPG_STRATEGY_ADDITION;
// 
// 			/* If asserts enabled, verify encoding of reconstructed string */
// 			Assert(pg_verifymbstr(fullValue, fullLen, false));
// 
// 			r = varstr_cmp(fullValue, fullLen,
// 						   VARDATA_ANY(query), queryLen,
// 						   PG_GET_COLLATION());
// 		}
// 		else
// 		{
// 			/* Non-collation-aware comparison */
// 			r = memcmp(fullValue, VARDATA_ANY(query), Min(queryLen, fullLen));
// 
// 			if (r == 0)
// 			{
// 				if (queryLen > fullLen)
// 					r = -1;
// 				else if (queryLen < fullLen)
// 					r = 1;
// 			}
// 		}
// 
// 		switch (strategy)
// 		{
// 			case BTLessStrategyNumber:
// 				res = (r < 0);
// 				break;
// 			case BTLessEqualStrategyNumber:
// 				res = (r <= 0);
// 				break;
// 			case BTEqualStrategyNumber:
// 				res = (r == 0);
// 				break;
// 			case BTGreaterEqualStrategyNumber:
// 				res = (r >= 0);
// 				break;
// 			case BTGreaterStrategyNumber:
// 				res = (r > 0);
// 				break;
// 			default:
// 				elog(ERROR, "unrecognized strategy number: %d",
// 					 in->scankeys[j].sk_strategy);
// 				res = false;
// 				break;
// 		}
// 
// 		if (!res)
// 			break;				/* no need to consider remaining conditions */
// 	}
// 
// 	PG_RETURN_BOOL(res);
// }
