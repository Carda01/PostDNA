drop table t;
drop table t2;
drop table t3;
drop TABLE test_typmod;
drop table km;
DROP TABLE kmers;
DROP TABLE qkmers;
DROP INDEX idxxxmm;

drop extension if exists postdna;

CREATE EXTENSION postdna;

-- Data Types 

CREATE TABLE t (id integer, dna dna, kmer kmer, qkmer qkmer);

INSERT INTO t VALUES
(0, 'CAT', 'taccc', 'CGTAN'),
(1, 'CATG', 'ttt', 'MNTA'),
(2, 'CATGG', 'cca', 'CCCGAAAA'),
(3, 'TACAGATA', 'TTAcca', 'RYBNCCCGGT'),
(5, 'TACAGATAA', 'ccaGG', 'VTA'),
(6, 'CAAAATAAGCGAAAT', 'CCcca', 'CG'),
(7, 'CATATATATAGGGGGAAAATTTTTTT', 'CCAcca', 'TTTGCD');

SELECT * from t;

INSERT INTO t VALUES
(8, 'CATATATATAGGGGGAAAATTTTTTT', 'CCAccATATATATAGGGGGAAAATTTTTTATATATATAGGGGGAAAATTTTTTa', 'TTTGCD');

INSERT INTO t VALUES
(8, 'CATATATATAGGGGGAAAATTTTTTT', 'CCAccATATATATA', 'TTTGGGGGGAAAATTTTTTATATATATAGGGGGAAAATTTTTTaGGGGGACD');

---------------------------------------------------------------------------------------------------------------------------

-- Type Modifier 

CREATE TABLE test_typmod (dna dna(10), kmer kmer(5));

INSERT INTO test_typmod (dna, kmer) VALUES ('ACGT', 'CGCCC');

SELECT * FROM test_typmod;

INSERT INTO test_typmod (dna) VALUES ('ACGTACGTACGTCC'); 

INSERT INTO test_typmod (kmer) VALUES ('ACGTACGTACGTCC');


CREATE TABLE test_typmod2 (kmer kmer(34));

---------------------------------------------------------------------------------------------------------------------------
--Length

SELECT kmer, length(kmer) AS kmer_length , 
dna, length(dna) AS dna_length, 
qkmer, length(qkmer) AS qkmer_length
FROM t;


---------------------------------------------------------------------------------------------------------------------------

-- generate kmers

SELECT * FROM generate_kmers('CTGAACCCGGATT', 8);

SELECT * FROM generate_kmers('CTGAAATCGCTTAT', 20);

SELECT * FROM generate_kmers('CTGAAATCGCTTATCTGAAATCGCTTATCTGAAATCGCTTATCTGAAATCGCTTATCTGAAATCGCTTAT', 33);


---------------------------------------------------------------------------------------------------------------------------


-- Contains

INSERT INTO t VALUES
(8, 'CATATATTT', 'TGCGCC', 'HGNNCC'),
(9, 'CATATTCGT', 'TGCTTCGTC', 'WSMBDCGNN'),
(10, 'CATATTCGT', 'CGCGCGCTT', 'NNNNNNNNN')
(11, 'CATATTCGT', 'CGCGCGCTT', 'GCTGG');


select qkmer, kmer from t where qkmer @> kmer;

select qkmer, kmer from t where qkmer_contains(qkmer , kmer);

select qkmer_contains('NNGCC' , 'CCTTC');


---------------------------------------------------------------------------------------------------------------------------

-- starts-with test


CREATE TABLE kmers (kmer kmer);
INSERT INTO kmers VALUES ('TACGAAACT'), ('ATACGATCCT'), ('TACGACTA'), ('TACGCTC');
SELECT * FROM kmers WHERE kmer_starts_with(kmer, 'TACGA');
SELECT * FROM kmers WHERE kmer ^@ 'TACG';


CREATE TABLE qkmers (qkmer qkmer);
INSERT INTO qkmers VALUES ('TAYCSGAAACT'), ('ATACGATCCT'), ('TAVCGACTA'), ('TAYCGCTC'), ('TAYCSGCTC');

SELECT * FROM qkmers WHERE qkmer_starts_with(qkmer, 'TAYCS'::qkmer);
SELECT * FROM qkmers WHERE qkmer ^@ 'TAV';


---------------------------------------------------------------------------------------------------------------------------

-- Counting 


SELECT k.kmer, count(k.kmer) FROM generate_kmers(
    'ACTCGATGCAGTCAGTCGATGCATCATCAT'
    , 3) AS k(kmer) GROUP BY k.kmer;

SELECT k.kmer, count(k.kmer) FROM generate_kmers(
    'ACTACTACT'
    , 3) AS k(kmer) GROUP BY k.kmer;

WITH kmers AS (
SELECT k.kmer, count(*)
FROM generate_kmers('CGTCGAAAAA', 4) AS k(kmer) GROUP BY k.kmer
)
SELECT sum(count) AS total_count,
count(*) AS distinct_count,
count(*) FILTER (WHERE count = 1) AS unique_count FROM kmers;


---------------------------------------------------------------------------------------------------------------------------

-- Canonicalize

CREATE TABLE t2 (kmer kmer);

INSERT INTO t2 (kmer)
SELECT 'ACTGCC'
FROM generate_series(1, 5000);

INSERT INTO t2 (kmer)
SELECT 'CTACTA'
FROM generate_series(1, 5000);

INSERT INTO t2 (kmer)
SELECT 'TAGTAG'
FROM generate_series(1, 5000);

INSERT INTO t2 (kmer)
SELECT 'TTTT'
FROM generate_series(1, 5000);


select kmer,kmer_canonicalize(kmer), count(*)
from t2
group by kmer;


with kmers as (select distinct kmer from t2) 
select distinct kmer_canonicalize(kmer), count(*)
from t2
where kmer_canonicalize(kmer) in (select kmer from kmers) 
group by kmer_canonicalize(kmer)
UNION
select distinct kmer, count(*)
from t2
where kmer_canonicalize(kmer) not in (select kmer from kmers) 
group by kmer
;
 


---------------------------------------------------------------------------------------------------------------------------

--index

CREATE TABLE t3 (kmer kmer);

select pg_relation_size('t3');

-- insert > 20 Milion values

INSERT INTO t3 (kmer)
SELECT 'ACTGCC'
FROM generate_series(1, 5000);

INSERT INTO t3 (kmer)
SELECT 'ACGGCT'
FROM generate_series(1, 5000000);

INSERT INTO t3 (kmer)
SELECT 'ACTCGT'
FROM generate_series(1, 4500000);

INSERT INTO t3 (kmer)
SELECT 'GGGGGG'
FROM generate_series(1, 10000000);

INSERT INTO t3 (kmer)
SELECT 'CTACTA'
FROM generate_series(1, 5000);



Explain analyze select * from t3 where kmer='CTACTA';

Explain analyze select * from t3 where kmer ^@ 'CTAC';

Explain analyze select * from t3 where 'NNNNNN' @> kmer;

CREATE INDEX idxxxmm ON t3 USING SPGIST(kmer);

SET enable_seqscan TO off;


Explain analyze select * from t3 where kmer='CTACTA';

Explain analyze select * from t3 where kmer ^@ 'CTAC';

Explain analyze select * from t3 where 'NNNNNN' @> kmer;

SET enable_seqscan TO on;
