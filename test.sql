drop table t;
drop table km;
drop extension if exists postdna;

create extension postdna;

CREATE TABLE t (id integer, dna dna, kmer kmer, qkmer qkmer);

INSERT INTO t VALUES
(0, 'CAT', 'taccc', 'CGTAN'),
(1, 'CATG', 'ttt', 'MNTA'),
(2, 'CATGG', 'cca', 'CCCGAAAA'),
(3, 'TACAGATA', 'TTAcca', 'RYBNCCCGGT'),
(5, 'TACAGATAA', 'ccaGG', 'VTA'),
(6, 'CAAAATAAGCGAAAT', 'CCcca', 'CG'),
(7, 'CATATATATAGGGGGAAAATTTTTTT', 'CCAcca', 'TTTGCD'),
(8, 'CATATATATAGGGGGAAAATTTTTTT', 'CCAcca', 'TTTGCD'),
(9, 'CATATATATAGGGGGAAAATTTTTTT', 'CCAcca', 'TTTGCD');

INSERT INTO t VALUES
(8, 'CATATATATAGGGGGAAAATTTTTTT', 'CCAccATATATATAGGGGGAAAATTTTTTATATATATAGGGGGAAAATTTTTTa', 'TTTGCD');

INSERT INTO t VALUES
(8, 'CATATATATAGGGGGAAAATTTTTTT', 'CCAccATATATATA', 'TTTGGGGGGAAAATTTTTTATATATATAGGGGGAAAATTTTTTaGGGGGACD');

INSERT INTO t VALUES
(8, 'CATATATATAGGGGGAAAATTTTTTT', 'CCAccATATATATA', 'aaaaa');

SELECT * FROM t;

SELECT * FROM generate_kmers('CTGAAATT', 4);

SELECT * FROM t where kmer = 'TTT';

SELECT * FROM generate_kmers('CTGAAATT', 4);

SELECT k.kmer, count(k.kmer) FROM generate_kmers('CTGAAAATTTTTT', 3) AS k(kmer) GROUP BY k.kmer;

create table km (kmer kmer);

INSERT INTO km VALUES
('TAC'),
('ACT'),
('CTA'),
('TAC'),
('ACT'),
('CTA'),
('TAC'),
('ACT'),
('CTA'),
('TAC');
