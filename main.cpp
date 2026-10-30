#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <iostream>
#include <string>
#include <fstream>
#include <regex>
#include <pqxx/pqxx>
#include <tgbot/tgbot.h>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

std::string fetch_api_data(const std::string& conn_str) {
    std::string report = "<b>Отчет парсера (JSON API):</b>\n\n";

    cpr::Header headers{
        {"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"},
        {"Accept", "application/json, text/plain, */*"}
    };

    cpr::Response r = cpr::Get(cpr::Url{"https://dummy-glass-market.by/api/v1/prices"}, headers);

    if (r.status_code != 200) {
        return report + "<b>Ошибка доступа:</b> код " + std::to_string(r.status_code);
    }

    try {
        json response_json = json::parse(r.text);
        
        if (response_json.contains("data") && !response_json["data"].empty()) {
            std::string product_name = response_json["data"][0]["name"];
            double price = response_json["data"][0]["price"];
            
            // Блок записи в PostgreSQL
            try {
                pqxx::connection C(conn_str);
                pqxx::work W(C);
                
                std::string sql = R"(INSERT INTO price_history (product_id, supplier_id, price) VALUES ($1, $2, $3);)";
                
                // Упаковка аргументов для современного метода exec
                W.exec(sql, pqxx::params{1, 1, price}); 
                W.commit();
                
                report += "Найдено: " + product_name + "\n";
                report += "Цена: <b>" + std::to_string(price) + " BYN</b>\n";
                report += "<i>Данные успешно записаны в БД.</i>";
            } catch (const std::exception& db_err) {
                report += "<b>Ошибка записи в БД:</b> " + std::string(db_err.what());
            }
            
        } else {
            report += "Данные не найдены или структура JSON изменилась.";
        }
    } catch (const json::exception& e) {
        report += "<b>Ошибка парсинга JSON:</b> " + std::string(e.what());
    }

    return report;
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    srand(static_cast<unsigned int>(time(NULL))); 

    // Чтение конфигурации БД
    std::string conn_str;
    std::ifstream db_file("db_config.txt");
    if (db_file.is_open()) {
        std::getline(db_file, conn_str);
        db_file.close();
    } else {
        std::cerr << "[ERROR] Файл db_config.txt не найден!" << std::endl;
        return 1;
    }

    // Чтение токена бота
    std::string bot_token;
    std::ifstream token_file("bot_token.txt");
    if (token_file.is_open()) {
        std::getline(token_file, bot_token);
        token_file.close();
    } else {
        std::cerr << "[ERROR] Файл bot_token.txt не найден!" << std::endl;
        return 1;
    }

    TgBot::Bot bot(bot_token);

    // Команда /start
    bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Привет! Я бот для мониторинга цен на стекло.\n\n/prices - Показать последние цены\n/update - Запустить парсер и обновить цены");
    });

    // Команда /prices
    bot.getEvents().onCommand("prices", [&bot, &conn_str](TgBot::Message::Ptr message) {
        try {
            pqxx::connection C(conn_str);
            pqxx::work W(C);
            pqxx::result R = W.exec("SELECT p.name, ph.price, ph.scraped_at FROM products p JOIN price_history ph ON p.id = ph.product_id ORDER BY ph.scraped_at DESC LIMIT 5;");
            
            std::string reply = "<b>Последние цены в базе:</b>\n\n";
            if (R.empty()) reply += "База пока пуста.";
            for (auto const& row : R) {
                reply += "🔹 " + std::string(row[0].c_str()) + ": <b>" + std::string(row[1].c_str()) + " BYN</b>\n";
            }
            bot.getApi().sendMessage(message->chat->id, reply, nullptr, nullptr, nullptr, "HTML");
        } catch (const std::exception&) {
            bot.getApi().sendMessage(message->chat->id, "Ошибка при запросе к БД.");
        }
    });

    bot.getEvents().onCommand("update", [&bot, &conn_str](TgBot::Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Запускаю сбор данных с сайтов поставщиков. Это может занять несколько секунд...");
        
        std::string report = fetch_api_data(conn_str);
        
        bot.getApi().sendMessage(message->chat->id, report, nullptr, nullptr, nullptr, "HTML");
    });

    try {
        std::cout << "[OK] Бот " << bot.getApi().getMe()->username << " запущен..." << std::endl;
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            longPoll.start();
        }
    } catch (TgBot::TgException& e) {
        std::cerr << "[ERROR] Ошибка Telegram API: " << e.what() << std::endl;
    }

    return 0;
}