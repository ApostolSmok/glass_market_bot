#!/bin/bash

echo "Остановка бота..."
sudo systemctl stop tgbot

cd /home/ubuntu/bot_project

echo "Компилируется новая версия..."
cd build
make

echo "Запуск службы..."
sudo systemctl start tgbot

echo "Бот успешно обновлен! Текущий статус:"
sudo systemctl status tgbot | grep "Active"