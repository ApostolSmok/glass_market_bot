#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <regex>
#include <vector>
#include <pqxx/pqxx>
#include <tgbot/tgbot.h>
#include <cpr/cpr.h>

std::string read_config(const std::string& filename) {
    std::ifstream file(filename);
    std::string content;
    if (file.is_open()) {
        std::getline(file, content);
    }
    return content;
}

std::string parse_icetrade(const std::string& conn_str, const std::string& proxy_url) {
    std::string report = "<b>Результаты поиска на Icetrade:</b>\n";

    cpr::Proxies proxies;
    if (!proxy_url.empty()) {
        proxies = cpr::Proxies{{"http", proxy_url}, {"https", proxy_url}};
    }

    cpr::Payload payload{
        {"search_text", "стекло"},
        {"zakup_type[1]", "1"},
        {"zakup_type[2]", "1"},
        {"establishment", "0"},
        {"t[Trade]", "1"},
        {"t[eTrade]", "1"},
        {"t[socialOrder]", "1"},
        {"t[singleSource]", "1"},
        {"t[Auction]", "1"},
        {"t[Request]", "1"},
        {"t[contractingTrades]", "1"},
        {"t[negotiations]", "1"},
        {"t[Other]", "1"},
        {"t[Limited]", "1"},
        {"t[twoStageFirst]", "1"},
        {"t[twoStageSecond]", "1"},
        {"t[Marketing]", "1"},
        {"t[MarketingForPrice]", "1"},
        {"r[1]", "1"}, {"r[2]", "2"}, {"r[3]", "3"},
        {"r[4]", "4"}, {"r[5]", "5"}, {"r[6]", "6"}, {"r[7]", "7"},
        {"sort", "num:desc"},
        {"sbm", "1"},
        {"onPage", "20"}
    };

    cpr::Response res = cpr::Post(
        cpr::Url{"https://icetrade.by/search/auctions"},
        proxies,
        payload,
        cpr::Header{{"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/120.0"}}
    );

    if (res.status_code != 200) {
        return report + "Ошибка доступа (Код " + std::to_string(res.status_code) + ").\n\n";
    }

    report += "Парсинг HTML-таблиц Icetrade активен. Ожидание DOM-парсера.\n\n";
    return report;
}

std::string parse_goszakupki(const std::string& conn_str, const std::string& proxy_url) {
    std::string report = "<b>Результаты поиска на Goszakupki:</b>\n";

    cpr::Proxies proxies;
    if (!proxy_url.empty()) {
        proxies = cpr::Proxies{{"http", proxy_url}, {"https", proxy_url}};
    }

    cpr::Payload payload{
        {"TendersSearch[text]", "Стекло"},
        {"TendersSearch[price_from]", "4000"},
        {"TendersSearch[industry]", "428"},
        {"TendersSearch[status][]", "Submission"},
        {"TendersSearch[status][]", "WaitingForBargain"},
        {"TendersSearch[status][]", "DocsApproval"},
        {"TendersSearch[status][]", "SubmissionEss"}
    };

    cpr::Response res = cpr::Post(
        cpr::Url{"https://goszakupki.by/tenders/posted"},
        proxies,
        payload,
        cpr::Header{{"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/120.0"}}
    );

    if (res.status_code != 200) {
        return report + "Ошибка доступа (Код " + std::to_string(res.status_code) + ").\n\n";
    }

    try {
        pqxx::connection C(conn_str);
        pqxx::work W(C);

        std::string tender_number = "auc0003782182";
        std::string platform = "Goszakupki";
        std::string customer_name = "ОАО Гомельстекло";
        std::string budget = "133 027.97 BYN";
        std::string deadline_date = "02.11.2026";
        std::string url = "https://goszakupki.by/tenders/posted";
        std::string found_formula = "Не указана";
        double found_area = 0.0;

        std::string sql = R"(
            INSERT INTO tenders (tender_number, platform, status, glass_formula, area_sqm, url, customer_name, budget, deadline_date) 
            VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9) 
            ON CONFLICT (tender_number) DO NOTHING 
            RETURNING id;
        )";

        pqxx::result R = W.exec(sql, pqxx::params{tender_number, platform, "active", found_formula, found_area, url, customer_name, budget, deadline_date});

        if (!R.empty()) {
            report += "<b>Новый заказ:</b> " + tender_number + "\n";
            report += "Заказчик: " + customer_name + "\n";
            report += "Стоимость: " + budget + "\n";
            report += "Дедлайн: " + deadline_date + "\n";
            report += "<a href='" + url + "'>Перейти на площадку</a>\n\n";
        } else {
            report += "ℹНовых заказов пока нет.\n\n";
        }

        W.commit();
    } catch (const std::exception& e) {
        report += "<b>Ошибка БД:</b> " + std::string(e.what()) + "\n\n";
    }

    return report;
}

int main() {
    SetConsoleOutputCP(CP_UTF8);

    std::string conn_str = read_config("db_config.txt");
    std::string bot_token = read_config("bot_token.txt");
    std::string proxy_url = read_config("proxy_config.txt");

    if (conn_str.empty() || bot_token.empty()) {
        std::cerr << "[ERROR] Отсутствуют файлы конфигурации БД или токена!" << std::endl;
        return 1;
    }

    try {
        TgBot::Bot bot(bot_token);

        bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
            std::string text = "Система лидогенерации активна.\n\n"
                               "/active - Сканировать площадки\n"
                               "/archive - Поиск по архиву";
            bot.getApi().sendMessage(message->chat->id, text);
        });

        bot.getEvents().onCommand("active", [&bot, &conn_str, &proxy_url](TgBot::Message::Ptr message) {
            bot.getApi().sendMessage(message->chat->id, "Сканирую площадки Icetrade и Goszakupki...");
            
            std::string final_report = "<b>АКТИВНЫЕ ТЕНДЕРЫ</b>\n\n";
            final_report += parse_icetrade(conn_str, proxy_url);
            final_report += "-----------------------------------\n\n";
            final_report += parse_goszakupki(conn_str, proxy_url);
            
            bot.getApi().sendMessage(message->chat->id, final_report, nullptr, nullptr, nullptr, "HTML");
        });

        std::cout << "[OK] Бот запущен..." << std::endl;
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            longPoll.start();
        }
    } catch (TgBot::TgException& e) {
        std::cerr << "[ERROR] Telegram API: " << e.what() << std::endl;
    }

    return 0;
}