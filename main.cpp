#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <string>
#include <fstream>
#include <regex>
#include <pqxx/pqxx>
#include <tgbot/tgbot.h>
#include <cpr/cpr.h>
#include <iomanip>
#include <sstream>

std::string read_config(const std::string& filename) {
    std::ifstream file(filename);
    std::string content;
    if (file.is_open()) {
        std::getline(file, content);
    }
    return content;
}

std::string get_view_state(const std::string& text) {
    std::smatch match;
    std::regex ajax_regex(R"-(<update id="javax\.faces\.ViewState"><!\[CDATA\[([^\]]+)\]\]></update>)-");
    if (std::regex_search(text, match, ajax_regex)) return match[1].str();
    
    std::regex html_regex(R"-(name="javax\.faces\.ViewState"[^>]*value="([^"]+)")-");
    if (std::regex_search(text, match, html_regex)) return match[1].str();
    
    return "";
}

std::string parse_butb(const std::string& conn_str, const std::string& proxy_url, int64_t chat_id) {
    std::string report = "<b>Результаты поиска на БУТБ (Стекло):</b>\n\n";
    bool found_new = false;
    cpr::Session session;

    session.SetUrl(cpr::Url{"https://zakupki.butb.by/auctions/reestrauctions.html"});
    if (!proxy_url.empty()) {
        session.SetProxies(cpr::Proxies{{"http", proxy_url}, {"https", proxy_url}});
    }
    
    session.SetHeader(cpr::Header{{"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/120.0"}});
    cpr::Response res1 = session.Get();
    if (res1.status_code != 200) return report + "Ошибка доступа к БУТБ.\n";

    std::string view_state = get_view_state(res1.text);
    if (view_state.empty()) return report + "Не удалось получить первичный ViewState.\n";

    std::string expand_id = "fra:j_idt213"; 
    std::smatch match;
    std::regex exp_reg(R"-(id="(fra:j_idt\d+)"[^>]*>Раскрыть форму поиска)-");
    if (std::regex_search(res1.text, match, exp_reg)) {
        expand_id = match[1].str();
    }

    session.SetHeader(cpr::Header{
        {"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/120.0"},
        {"Faces-Request", "partial/ajax"}
    });
    
    session.SetPayload(cpr::Payload{
        {"fra", "fra"},
        {"javax.faces.ViewState", view_state},
        {"javax.faces.source", expand_id},
        {"javax.faces.partial.event", "click"},
        {"javax.faces.partial.execute", "@all"},
        {"javax.faces.partial.render", "@all"},
        {"fra:j_idcl", expand_id},
        {"ice.submit.type", "ice.s"},
        {"javax.faces.partial.ajax", "true"}
    });
    
    cpr::Response res2 = session.Post();
    
    std::string new_vs = get_view_state(res2.text);
    if (!new_vs.empty()) view_state = new_vs;

    std::string search_field = "fra:j_idt222";
    std::string okrb_field = "fra:j_idt252";
    std::string search_btn = "fra:_t320";

    std::regex sf_reg(R"-(Предмет закупки[\s\S]{0,300}?name="(fra:j_idt\d+)")-");
    if (std::regex_search(res2.text, match, sf_reg)) search_field = match[1].str();

    std::regex okrb_reg(R"-(Код ОКРБ[\s\S]{0,300}?name="(fra:j_idt\d+)")-");
    if (std::regex_search(res2.text, match, okrb_reg)) okrb_field = match[1].str();

    std::regex btn_reg(R"-(id="(fra:[^"]+)"[^>]*>Искать)-");
    if (std::regex_search(res2.text, match, btn_reg)) search_btn = match[1].str();

    session.SetPayload(cpr::Payload{
        {"fra", "fra"},
        {search_field, "стекло"},
        {"fra:selProduct", "1"},
        {okrb_field, "23.12.13.300"},
        {"fra:_t328:0:_t331", "on"},
        {"fra:_t328:1:_t331", "on"},
        {"fra:_t328:2:_t331", "on"},
        {"fra:_t328:3:_t331", "on"},
        {"fra:_t328:4:_t331", "on"},
        {"fra:_t328:6:_t331", "on"},
        {"fra:_t328:7:_t331", "on"},
        {"fra:_t328:8:_t331", "on"},
        {"fra:_t328:9:_t331", "on"},
        {"fra:_t328:10:_t331", "on"},
        {"javax.faces.ViewState", view_state},
        {"javax.faces.source", search_btn},
        {"javax.faces.partial.event", "click"},
        {"javax.faces.partial.execute", "@all"},
        {"javax.faces.partial.render", "@all"},
        {"ice.submit.type", "ice.s"},
        {"javax.faces.partial.ajax", "true"}
    });

    cpr::Response res3 = session.Post();

    try {
        pqxx::connection C(conn_str);
        pqxx::work W(C);

        std::regex safe_regex(R"-(>([A-Z]+\d+)[\s\S]*?>([\d&#;,\s]+)</span>[\s\S]*?BYN)-");
        
        size_t pos = 0;
        while ((pos = res3.text.find("<tr", pos)) != std::string::npos) {
            size_t end_pos = res3.text.find("</tr>", pos);
            if (end_pos == std::string::npos) break;
            
            std::string row_html = res3.text.substr(pos, end_pos - pos);
            pos = end_pos + 5;
            
            std::smatch sm;
            if (!std::regex_search(row_html, sm, safe_regex)) continue;

            std::string tender_number = sm[1].str();
            std::string raw_price = sm[2].str();

            raw_price = std::regex_replace(raw_price, std::regex(R"((BYN|byn|&nbsp;|&#160;|\s|[A-Za-zА-Яа-я]))"), "");
            std::replace(raw_price.begin(), raw_price.end(), ',', '.');
            
            double price_value = 0.0;
            try {
                if (!raw_price.empty()) {
                    price_value = std::stod(raw_price);
                }
            } catch (const std::exception&) {
                continue; 
            }

            if (price_value >= 4000.0) {
                std::string sql = R"(
                    INSERT INTO tenders (tender_number, platform, status, budget, url, chat_id) 
                    VALUES ($1, $2, $3, $4, $5, $6) 
                    ON CONFLICT (tender_number, chat_id) DO NOTHING 
                    RETURNING id;
                )";

                std::string tender_url = "https://zakupki.butb.by/auctions/reestrauctions.html" + tender_number;
                pqxx::result R = W.exec(sql, pqxx::params{tender_number, "BUTB", "См. на сайте", std::to_string(price_value), tender_url, chat_id});

                if (!R.empty()) {
                    std::stringstream stream;
                    stream << std::fixed << std::setprecision(2) << price_value;
                    std::string formatted_price = stream.str();
                    report += "<b>Тендер:</b> <code>" + tender_number + "</code>\n";
                    report += "Сумма: <b>" + formatted_price + " BYN</b>\n";
                    report += "<a href='" + tender_url + "'>Перейти на БУТБ</a>\n\n";
                    
                    found_new = true;
                }
            }
        }
        W.commit();
    } catch (const std::exception& e) {
        return report + "<b>Ошибка БД:</b> " + std::string(e.what()) + "\n";
    }

    if (!found_new) report += "Подходящих новых тендеров на стекло от 4000 BYN не найдено.\n";
    return report;
}

int main() {
    std::string conn_str = read_config("db_config.txt");
    std::string bot_token = read_config("bot_token.txt");
    std::string proxy_url = read_config("proxy_config.txt");

    if (conn_str.empty() || bot_token.empty()) {
        std::cerr << "[ERROR] Отсутствуют файлы конфигурации!" << std::endl;
        return 1;
    }

    try {
        TgBot::Bot bot(bot_token);

        bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
            std::string text = "Бот для парсинга БУТБ запущен.\n\n/active - Сканировать площадку";
            bot.getApi().sendMessage(message->chat->id, text);
        });

        bot.getEvents().onCommand("active", [&bot, &conn_str, &proxy_url](TgBot::Message::Ptr message) {
            bot.getApi().sendMessage(message->chat->id, "Сканирую БУТБ с применением фильтров, ожидайте...");
            std::string final_report = parse_butb(conn_str, proxy_url, message->chat->id);
            
            bot.getApi().sendMessage(message->chat->id, final_report, nullptr, nullptr, nullptr, "HTML");
        });

        std::cout << "[OK] Бот запущен" << std::endl;
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            longPoll.start();
        }
    } catch (TgBot::TgException& e) {
        std::cerr << "[ERROR] Telegram API: " << e.what() << std::endl;
    }

    return 0;
}