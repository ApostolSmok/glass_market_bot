CREATE TABLE tenders (
    id SERIAL PRIMARY KEY,
    tender_number VARCHAR(100) UNIQUE NOT NULL, 
    platform VARCHAR(50) NOT NULL,              
    status VARCHAR(20) NOT NULL,                
    glass_formula TEXT,                         
    area_sqm NUMERIC(10, 2),                    
    url TEXT NOT NULL,                          
    published_at DATE,                          
    scraped_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

ALTER TABLE tenders ADD COLUMN customer_name VARCHAR(255);
ALTER TABLE tenders ADD COLUMN budget VARCHAR(100); 
ALTER TABLE tenders ADD COLUMN deadline_date VARCHAR(50);