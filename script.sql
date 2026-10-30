-- 1. Таблица товаров 
CREATE TABLE IF NOT EXISTS products (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    category VARCHAR(100),
    thickness_mm NUMERIC(4,2),
    manufacturer VARCHAR(150),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 2. Таблица поставщиков
CREATE TABLE IF NOT EXISTS suppliers (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    website VARCHAR(255),
    contact_info TEXT
);

-- 3. Таблица истории цен со связями
CREATE TABLE IF NOT EXISTS price_history (
    id BIGSERIAL PRIMARY KEY,
    product_id INT NOT NULL,
    supplier_id INT NOT NULL,
    price NUMERIC(10,2) NOT NULL,
    currency VARCHAR(3) DEFAULT 'BYN',
    is_in_stock BOOLEAN DEFAULT TRUE,
    scraped_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (product_id) REFERENCES products(id) ON DELETE CASCADE,
    FOREIGN KEY (supplier_id) REFERENCES suppliers(id) ON DELETE CASCADE
);