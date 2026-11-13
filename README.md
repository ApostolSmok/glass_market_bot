# Glass Market Bot (glass_market_bot)

## Overview
Telegram bot backend written in C++ designed for monitoring and parsing procurement tenders from the Belarusian Universal Commodity Exchange (BUTB) and the PPT trading platform. The bot tracks tenders related to glass products with a budget exceeding 4000 BYN. Filtering is executed based on OKRB code 23.12.13.300 for the main BUTB platform and TN VED codes 7007 and 7008 for the PPT platform.

Live Bot Username: [@glassmarket1235_bot](https://t.me/glassmarket1235_bot)

## Key Features
*   **Dual-Strategy Automated Parsing:** Bypasses dynamic JS/JSF forms on the BUTB platform using simulated HTTP POST/GET requests and ViewState management, while seamlessly interacting with the PPT platform via RESTful JSON API payloads.
*   **Memory-Safe Processing:** Utilizes custom string-chunking algorithms combined with regex to parse large HTML payloads without stack overflow issues.
*   **Interactive UI:** Implements Telegram Reply and Inline Keyboards for seamless user interaction and on-demand scanning triggers without relying on manual command input.
*   **Persistent Storage:** Integrates with PostgreSQL to store tender history and prevent duplicate notifications.
*   **Proxy Support:** Configurable HTTP/HTTPS proxy support for network environments requiring proxy routing.
*   **System Integration:** Designed to run continuously as a systemd service on Linux (Ubuntu) environments.
*   **Automated Maintenance:** Includes shell scripts for daily database backups and streamlined deployment updates.

## Prerequisites
The system is designed to be deployed on an Ubuntu-based server.
*   **Compiler:** GCC supporting C++20 standard.
*   **Build Systems:** CMake (>= 3.10), Make, vcpkg (C++ Package Manager).
*   **Database:** PostgreSQL.
*   **Libraries (via vcpkg):** `tgbot-cpp`, `cpr`, `libpqxx`, `nlohmann-json`.

## Configuration
Before compiling and running, you must create the following configuration files in the root directory (these are excluded via `.gitignore` for security):

1.  `bot_token.txt`: Contains your Telegram Bot API token.
2.  `db_config.txt`: Contains the PostgreSQL connection string (e.g., `dbname=butb_db user=botuser password=yourpassword host=127.0.0.1 port=5432`).
3.  `proxy_config.txt`: Contains the proxy URL (e.g., `http://user:pass@ip:port`). Leave the file empty if a direct connection is used.

**Note:** Ensure these files use UNIX line endings (`\n`), avoiding Windows `\r\n` artifacts.

## Database Setup
1. Switch to the postgres user and open the console:
   ```bash
   sudo -u postgres psql
   ```
2. Execute the commands found in the second part of `script.sql` to initialize the `butb_db` database, create the `botuser`, assign privileges, and create the `tenders` table.

## Compilation Instructions
The project utilizes `vcpkg` for dependency management. 

```bash
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
make
```

Ensure configuration `.txt` files are copied to the `build` directory before executing the binary.

## Deployment & Systemd Service
To ensure 24/7 uptime, configure the application as a `systemd` service.

Create a file at `/etc/systemd/system/tgbot.service`:
```ini
[Unit]
Description=Glass Market Telegram Bot
After=network.target network-online.target

[Service]
Type=simple
User=ubuntu
WorkingDirectory=/home/ubuntu/bot_project/build
ExecStart=/home/ubuntu/bot_project/build/backend_app
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

Enable and start the service:
```bash
sudo systemctl daemon-reload
sudo systemctl enable tgbot
sudo systemctl start tgbot
```

Logs can be monitored via: `sudo journalctl -u tgbot -f`.

## Maintenance Scripts
The repository includes utility scripts for server management:
*   `backup_db_2.sh`: Executes daily PostgreSQL dumps via `cron` and automatically purges backups older than 7 days.
*   `update_bot.sh`: Halts the systemd service, recompiles the backend from the latest source code, and restarts the service.