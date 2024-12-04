CREATE EXTENSION postdna;

CREATE TABLE dna_sequences (
    id SERIAL PRIMARY KEY,
    name TEXT,
    sequence dna
);

CREATE TABLE kmers (
    id SERIAL PRIMARY KEY,
    kmer kmer,
    sequence_id INTEGER REFERENCES dna_sequences(id)
);

Select * FROM dna_sequences;


-- Generate and insert k-mers
INSERT INTO kmers (kmer, sequence_id)
SELECT k.kmer, s.id
FROM dna_sequences s,
     generate_kmers(s.sequence, 5) AS k(kmer);

-- Test length functions
SELECT name, length(sequence) AS dna_length
FROM dna_sequences;

SELECT DISTINCT kmer, length(kmer) AS kmer_length
FROM kmers
LIMIT 5;

-- Test k-mer equality
SELECT COUNT(*) AS matching_kmers
FROM kmers
WHERE kmer = 'ACGTA'::kmer;

-- Test k-mer prefix matching
SELECT DISTINCT kmer
FROM kmers
WHERE kmer ^@ 'ACG'::kmer;

/*ERROR:  operator does not exist: kmer ^@ kmer
LINE 4: WHERE kmer ^@ 'ACG'::kmer;
                   ^
HINT:  No operator matches the given name and argument types. You might need to add explicit type casts. 

SQL state: 42883
Character: 51*/

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

-- Test k-mer containment with qkmer
SELECT DISTINCT kmer
FROM kmers
WHERE 'ANGTA'::qkmer @> kmer;


-- Most frequent k-mers
SELECT kmer, COUNT(*) AS frequency
FROM kmers
GROUP BY kmer
ORDER BY frequency DESC
LIMIT 10;

-- K-mer frequency per sequence
SELECT s.name, k.kmer, COUNT(*) AS frequency
FROM kmers k
JOIN dna_sequences s ON k.sequence_id = s.id
GROUP BY s.name, k.kmer
ORDER BY s.name, frequency DESC
LIMIT 10;

-- Create SP-GiST index
CREATE INDEX kmer_spgist_idx ON kmers USING spgist (kmer);

/*ERROR:  data type kmer has no default operator class for access method "spgist"
HINT:  You must specify an operator class for the index or define a default operator class for the data type. 

SQL state: 42704*/

-- Test index performance
EXPLAIN ANALYZE
SELECT kmer
FROM kmers
WHERE kmer = 'ACGTA'::kmer;

EXPLAIN ANALYZE
SELECT kmer
FROM kmers
WHERE kmer ^@ 'ACG'::kmer;

EXPLAIN ANALYZE
SELECT kmer
FROM kmers
WHERE 'ANGTA'::qkmer @> kmer;

-- Clean up
DROP TABLE kmers;
DROP TABLE dna_sequences;
DROP INDEX kmer_spgist_idx;