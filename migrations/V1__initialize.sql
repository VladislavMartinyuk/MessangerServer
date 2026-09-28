CREATE TABLE users (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    uuid CHAR(36) NOT NULL UNIQUE,
    login VARCHAR(255) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    name VARCHAR(50) NOT NULL,
    date_birth DATE NOT NULL,
    created_at DATETIME(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    updated_at DATETIME(6),
    deleted_at DATETIME(6)
);

CREATE TABLE chat_types (
    id BIGINT UNSIGNED NOT NULL PRIMARY KEY,
    name VARCHAR(36) NOT NULL UNIQUE
);

INSERT INTO chat_types (id, name) VALUES (1, 'personal'), (2, 'group');

CREATE TABLE chat (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    uuid CHAR(36) NOT NULL UNIQUE,
    name VARCHAR(100) NOT NULL,
    type_id BIGINT UNSIGNED NOT NULL,
    created_by CHAR(36) NOT NULL,
    -- Sorted UUIDs make creating the same personal chat idempotent.
    personal_key VARCHAR(73) UNIQUE,
    created_at DATETIME(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    updated_at DATETIME(6),
    deleted_at DATETIME(6),
    FOREIGN KEY (type_id) REFERENCES chat_types(id),
    FOREIGN KEY (created_by) REFERENCES users(uuid)
);

CREATE TABLE user_chats (
    user_uuid CHAR(36) NOT NULL,
    chat_uuid CHAR(36) NOT NULL,
    PRIMARY KEY (user_uuid, chat_uuid),
    INDEX ix_user_chats_chat (chat_uuid),
    FOREIGN KEY (user_uuid) REFERENCES users(uuid),
    FOREIGN KEY (chat_uuid) REFERENCES chat(uuid) ON DELETE CASCADE
);

CREATE TABLE messages (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    uuid CHAR(36) NOT NULL UNIQUE,
    chat_uuid CHAR(36) NOT NULL,
    sender_uuid CHAR(36) NOT NULL,
    message_text TEXT NOT NULL,
    created_at DATETIME(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    updated_at DATETIME(6),
    deleted_at DATETIME(6),
    INDEX ix_messages_chat_id (chat_uuid, id),
    FOREIGN KEY (chat_uuid) REFERENCES chat(uuid) ON DELETE CASCADE,
    FOREIGN KEY (sender_uuid) REFERENCES users(uuid)
);
