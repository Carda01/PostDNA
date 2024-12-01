DO
$$
DECLARE
    table_name text;
BEGIN
    -- Loop through all tables with columns of the specified type
    FOR table_name IN
        SELECT DISTINCT
            c.table_schema || '.' || c.table_name AS full_table_name
        FROM
            information_schema.columns c
        WHERE
            c.udt_name in ('dna', 'kmer', 'qkmer')
    LOOP
        -- Drop the table
        EXECUTE format('DROP TABLE %s CASCADE', table_name);
        RAISE NOTICE 'Dropped table: %', table_name;
    END LOOP;
END
$$;

drop extension if exists postdna;
