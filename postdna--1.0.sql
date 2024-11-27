-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION postdna" to load this file. \quit

/******************************************************************************
 * Input/Output
 ******************************************************************************/

-- DNA

CREATE OR REPLACE FUNCTION dna_in(cstring)
  RETURNS dna
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION dna_out(dna)
  RETURNS cstring
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE TYPE dna (
  input          = dna_in,
  output         = dna_out
);

COMMENT ON TYPE dna IS 'sequence of nucleotids (ACGT) without a max length';

CREATE OR REPLACE FUNCTION dna(text)
  RETURNS dna
  AS 'MODULE_PATHNAME', 'dna_cast_from_text'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION text(dna)
  RETURNS text
  AS 'MODULE_PATHNAME', 'dna_cast_to_text'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE CAST (text as dna) WITH FUNCTION dna(text) AS IMPLICIT;
CREATE CAST (dna as text) WITH FUNCTION text(dna);



-- Kmer

CREATE OR REPLACE FUNCTION kmer_in(cstring)
  RETURNS kmer
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION kmer_out(kmer)
  RETURNS cstring
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE TYPE kmer (
  input          = kmer_in,
  output         = kmer_out
);

COMMENT ON TYPE kmer IS 'sequence of nucleotids (ACGT) without a max length';

CREATE OR REPLACE FUNCTION kmer(text)
  RETURNS kmer
  AS 'MODULE_PATHNAME', 'kmer_cast_from_text'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION text(kmer)
  RETURNS text
  AS 'MODULE_PATHNAME', 'kmer_cast_to_text'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE CAST (text as kmer) WITH FUNCTION kmer(text) AS IMPLICIT;
CREATE CAST (kmer as text) WITH FUNCTION text(kmer);


CREATE OR REPLACE FUNCTION generate_kmers(dna, integer)
    RETURNS SETOF kmer
    AS 'MODULE_PATHNAME', 'generate_kmers'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- QKmer

CREATE OR REPLACE FUNCTION qkmer_in(cstring)
  RETURNS qkmer
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION qkmer_out(qkmer)
  RETURNS cstring
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE TYPE qkmer (
  input          = qkmer_in,
  output         = qkmer_out
);

COMMENT ON TYPE qkmer IS 'sequence of Query Symbols with maximum length of 32.';

CREATE OR REPLACE FUNCTION qkmer(text)
  RETURNS qkmer
  AS 'MODULE_PATHNAME', 'qkmer_cast_from_text'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION text(qkmer)
  RETURNS text
  AS 'MODULE_PATHNAME', 'qkmer_cast_to_text'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE CAST (text as qkmer) WITH FUNCTION qkmer(text) AS IMPLICIT;
CREATE CAST (qkmer as text) WITH FUNCTION text(qkmer);


-- *********************************************************************** --
-- KMERS



CREATE FUNCTION kmer_eq(kmer, kmer)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'kmer_equals'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION kmer_ne(kmer, kmer)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'kmer_nequals'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION kmer_hash(kmer) 
  RETURNS integer
  AS 'MODULE_PATHNAME', 'kmer_hash'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR = (
  LEFTARG = kmer, 
  RIGHTARG = kmer,
  PROCEDURE = kmer_eq,
  COMMUTATOR = '=',
  NEGATOR = '<>',
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);
COMMENT ON OPERATOR =(kmer, kmer) IS 'equals?';

CREATE OPERATOR <> (
  LEFTARG = kmer,
  RIGHTARG = kmer,
  PROCEDURE = kmer_ne,
  COMMUTATOR = '<>',
  NEGATOR = '=',
  RESTRICT = neqsel,
  JOIN = neqjoinsel
);

COMMENT ON OPERATOR <>(kmer,kmer) IS 'not equals?';

CREATE OPERATOR CLASS kmer_ops DEFAULT FOR TYPE kmer USING hash AS
    OPERATOR 1 = (kmer, kmer),
    FUNCTION 1 kmer_hash(kmer);

------------------------------------------------------------------------------
-----------------------------QKMER---------------------------------------------
------------------------------------------------------------------------------

CREATE FUNCTION qkmer_eq(qkmer, qkmer)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'qkmer_equals'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION qkmer_contains(qkmer, kmer)
  RETURNS boolean
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR = (
  LEFTARG = qkmer, 
  RIGHTARG = qkmer,
  PROCEDURE = qkmer_eq,
  COMMUTATOR = '=',
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);
COMMENT ON OPERATOR =(qkmer, qkmer) IS 'equals?';

CREATE OPERATOR @> (
  LEFTARG = qkmer, 
  RIGHTARG = kmer,
  PROCEDURE = qkmer_contains
);
COMMENT ON OPERATOR @>(qkmer, kmer) IS 'contains?';

-- *********************************************************************** --
-- Lengths Functions 

CREATE FUNCTION length(dna) RETURNS integer
    AS 'MODULE_PATHNAME', 'dna_length'
    LANGUAGE C STRICT;

CREATE FUNCTION length(kmer) RETURNS integer
    AS 'MODULE_PATHNAME', 'kmer_length'
    LANGUAGE C STRICT;

CREATE FUNCTION length(qkmer) RETURNS integer
    AS 'MODULE_PATHNAME', 'qkmer_length'
    LANGUAGE C STRICT;
