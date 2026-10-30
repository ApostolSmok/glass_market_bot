#include <iostream>
#include <string>
#include <windows.h>
#include <pqxx/pqxx>
#include <tgbot/tgbot.h>
#include <fstream>

int main() {
    SetConsoleOutputCP(CP_UTF8);

    std::string conn_str;
    std::ifstream db_file("db_config.txt");
    
    if (db_file.is_open()) {
        std::getline(db_file, conn_str);
        db_file.close();
    } else {
        std::cerr << "[ERROR] Не удалось найти файл db_config.txt! Убедись, что он лежит в корне проекта." << std::endl;
        return 1;
    }
    try {
        pqxx::connection C(conn_str);
        if (C.is_open()) {
            std::cout << "[OK] Подключение к БД: " << C.dbname() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Ошибка БД: " << e.what() << std::endl;
        return 1;
    }

    std::string bot_token;
    std::ifstream token_file("bot_token.txt");
    
    if (token_file.is_open()) {
        std::getline(token_file, bot_token);
        token_file.close();
    } else {
        std::cerr << "[ERROR] Не удалось найти файл bot_token.txt! Убедись, что он лежит рядом с программой или в корне проекта." << std::endl;
        return 1; 
    }
    TgBot::Bot bot(bot_token);

    bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Привет! Я система аналитики рынка стеклоизделий. Напиши /prices для проверки базы.");
    });

    bot.getEvents().onCommand("prices", [&bot, &conn_str](TgBot::Message::Ptr message) {
        try {
            pqxx::connection C(conn_str);
            pqxx::work W(C);
            pqxx::result R = W.exec("SELECT p.name, ph.price FROM products p JOIN price_history ph ON p.id = ph.product_id LIMIT 5;");
            
            std::string reply = "Последние цены в базе:\n";
            if (R.empty()) reply += "База пока пуста.";
            for (auto const& row : R) {
                reply += "- " + std::string(row[0].c_str()) + ": " + std::string(row[1].c_str()) + " BYN\n";
            }
            bot.getApi().sendMessage(message->chat->id, reply);
        } catch (const std::exception& e) {
            bot.getApi().sendMessage(message->chat->id, "Ошибка при запросе к БД.");
        }
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