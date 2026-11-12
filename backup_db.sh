#!/bin/bash
BACKUP_DIR="/home/ubuntu/bot_backups"
DATE=$(date +"%Y-%m-%d")

sudo -u postgres pg_dump butb_db > "$BACKUP_DIR/butb_db_$DATE.sql"

find "$BACKUP_DIR" -type f -name "*.sql" -mtime +7 -exec rm {} \;