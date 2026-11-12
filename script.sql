CREATE TABLE tenders (
    id SERIAL,
    tender_number VARCHAR(100),
    platform VARCHAR(50),
    status VARCHAR(50),
    budget VARCHAR(100),
    url VARCHAR(255),
    chat_id BIGINT,
    PRIMARY KEY (tender_number, chat_id)
);

SELECT * FROM tenders;

--# ON SERVER:

--# sudo -u postgres psql

CREATE DATABASE butb_db;
CREATE USER botuser WITH ENCRYPTED PASSWORD ' ';
GRANT ALL PRIVILEGES ON DATABASE butb_db TO botuser;
\c butb_db;

CREATE TABLE tenders (
    id SERIAL,
    tender_number VARCHAR(100),
    platform VARCHAR(50),
    status VARCHAR(50),
    budget VARCHAR(100),
    url VARCHAR(255),
    chat_id BIGINT,
    PRIMARY KEY (tender_number, chat_id)
);

GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO botuser;
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO botuser;
\q