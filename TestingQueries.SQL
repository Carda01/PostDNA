-- Enable the postdna extension
drop extension if exists postdna cascade;
CREATE EXTENSION IF NOT EXISTS postdna;

-- Drop tables if they exist
DROP TABLE IF EXISTS kmers;
DROP TABLE IF EXISTS dna_sequences;

-- Create dna_sequences table
CREATE TABLE dna_sequences (
    seq_id TEXT PRIMARY KEY,
    seq DNA NOT NULL
);


-- Verify the table creation
SELECT * FROM dna_sequences;

-- Create kmers table
CREATE TABLE kmers (
    id SERIAL PRIMARY KEY,
    kmer kmer NOT NULL,
    seq_id TEXT REFERENCES dna_sequences(seq_id)
);

-- Generate and insert k-mers
INSERT INTO kmers (kmer, seq_id)
SELECT k.kmer, s.seq_id
FROM dna_sequences s,
     generate_kmers(s.seq, 5) AS k(kmer);
	 
--Verify the table creation
SELECT * FROM kmers;


-- Test length functions
SELECT seq_id, length(seq) AS dna_length
FROM dna_sequences;

SELECT DISTINCT kmer, length(kmer) AS kmer_length
FROM kmers
LIMIT 5;

-- Test k-mer equality
SELECT COUNT(*) AS matching_kmers
FROM kmers
WHERE kmer = 'GTGTA'::kmer;

-- Test k-mer prefix matching (using the correct operator)
SELECT DISTINCT kmer
FROM kmers
WHERE 'ANGTA' @> kmer;  -- Correct operator for prefix matching

-- Test starts_with with qkmer
SELECT * FROM kmers WHERE kmer ^@ 'TACG';


-- Test pattern matching with qkmer
SELECT DISTINCT kmer
FROM kmers
WHERE 'ANGTA'::qkmer @> kmer;

-- Test k-mer counting
SELECT kmer, COUNT(*) AS frequency
FROM kmers
GROUP BY kmer
ORDER BY frequency DESC
LIMIT 10;

-- Most frequent k-mers
SELECT kmer, COUNT(*) AS frequency
FROM kmers
GROUP BY kmer
ORDER BY frequency DESC
LIMIT 10;

-- K-mer frequency per sequence
SELECT s.seq_id, k.kmer, COUNT(*) AS frequency
FROM kmers k
JOIN dna_sequences s ON k.seq_id = s.seq_id
GROUP BY s.seq_id, k.kmer
ORDER BY s.seq_id, frequency DESC
LIMIT 10;

-- Create SP-GiST index
-- CREATE INDEX kushari ON kmers USING SPGIST(kmer);
-- select * from kmers where kmer='ACTCA';


CREATE INDEX kmer_spgist_idx ON kmers USING spgist (kmer);
set enable_seqscan = off;
-- Test index performance
EXPLAIN ANALYZE
SELECT kmer
FROM kmers
WHERE kmer = 'ACGTA'::kmer;

-- Test starts_with with qkmer
EXPLAIN ANALYZE
SELECT kmer 
FROM kmers 
WHERE kmer ^@ 'ACGT';

EXPLAIN ANALYZE
SELECT kmer
FROM kmers
WHERE 'ACG' @> kmer;

EXPLAIN ANALYZE
SELECT kmer
FROM kmers
WHERE 'ANGTA'::qkmer @> kmer;
set enable_seqscan = on;


-- -- Clean up
-- DROP TABLE IF EXISTS kmers;
-- DROP TABLE IF EXISTS dna_sequences;
-- DROP INDEX IF EXISTS kmer_spgist_idx;
