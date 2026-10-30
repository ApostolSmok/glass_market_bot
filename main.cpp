#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include <windows.h>

int main() {
    SetConsoleOutputCP(CP_UTF8);
    try {
        //(имя базы, пользователь, пароль)
        std::string connection_string = "dbname=glass_market user=postgres password=1235 host=127.0.0.1 port=5432";
        pqxx::connection C(connection_string);
        
        if (C.is_open()) {
            std::cout << "Успешное подключение к БД: " << C.dbname() << std::endl;
        } else {
            std::cerr << "Ошибка подключения к БД" << std::endl;
            return 1;
        }

        pqxx::work W(C);

        std::string sql = R"(
            SELECT p.name, p.thickness_mm, ph.price, ph.currency 
            FROM products p
            JOIN price_history ph ON p.id = ph.product_id
            ORDER BY ph.scraped_at DESC
            LIMIT 5;
        )";

        pqxx::result R = W.exec(sql);

        std::cout << "Актуальные цены на стекло:" << std::endl;
        for (auto const& row : R) {
            std::string name = row[0].c_str();
            double thickness = row[1].as<double>();
            double price = row[2].as<double>();
            std::string currency = row[3].c_str();

            std::cout << "Товар: " << name << " (" << thickness << " мм) "
                      << "- Цена: " << price << " " << currency << std::endl;
        }
        
        W.commit();
        
    } catch (const pqxx::sql_error& e) {
        std::cerr << "SQL Ошибка: " << e.what() << "\nЗапрос: " << e.query() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Общая ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}