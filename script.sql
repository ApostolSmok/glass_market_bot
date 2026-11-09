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