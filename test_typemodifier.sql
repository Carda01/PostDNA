drop TABLE test_typmod;
drop TABLE test_typmod2;

drop extension if exists postdna;
create extension postdna;

CREATE TABLE test_typmod (seq dna(10));

INSERT INTO test_typmod (seq) VALUES ('ACGT'); -- Should work
INSERT INTO test_typmod (seq) VALUES ('ACGTACGTACGT'); -- Should fail with an error

SELECT * from test_typmod;


CREATE TABLE test_typmod2 (seq kmer(5));

INSERT INTO test_typmod2 (seq) VALUES ('ACGTC'); -- Should work
INSERT INTO test_typmod2 (seq) VALUES ('ACGTACGTACGT'); -- Should fail with an error

SELECT * from test_typmod2;

CREATE TABLE test_typmod3 (seq kmer(33)); -- Should fail
