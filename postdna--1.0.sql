-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION postdna" to load this file. \quit

/******************************************************************************
 * Input/Output
 ******************************************************************************/

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
