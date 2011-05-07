-- This queries for users PostgreSQL.
--
-- Its need apply single time after DB creating or later (safe reapply)
-- and let correctly execute SQL queries with UNIX_TIMESTAMP/FROM_UNIXTIME
-- functions uses in mangosd/realmd code.

-- NOTE: Queries must be applied to each mangos DB parts: mangos, characters, realmd at least one time.

-- no params
CREATE OR REPLACE FUNCTION unix_timestamp() RETURNS bigint AS '
    SELECT EXTRACT(EPOCH FROM CURRENT_TIMESTAMP(0))::bigint AS result;
' LANGUAGE 'SQL';

-- timestamp without time zone (i.e. 1973-11-29 21:33:09)
CREATE OR REPLACE FUNCTION unix_timestamp(timestamp) RETURNS bigint AS '
    SELECT EXTRACT(EPOCH FROM $1)::bigint AS result;
' LANGUAGE 'SQL';

-- timestamp with time zone (i.e. 1973-11-29 21:33:09+01)
CREATE OR REPLACE FUNCTION unix_timestamp(timestamp WITH time zone) RETURNS bigint AS '
    SELECT EXTRACT(EPOCH FROM $1)::bigint AS result;
' LANGUAGE 'SQL';

CREATE OR REPLACE FUNCTION from_unixtime(integer) RETURNS timestamp AS '
    SELECT to_timestamp($1)::timestamp AS result
' LANGUAGE 'SQL';
