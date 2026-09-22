CREATE TABLE
    IF NOT EXISTS users (
        id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
        uuid CHAR(36) NOT NULL,
        login VARCHAR(255) NOT NULL,
        password_hash VARCHAR(255) NOT NULL,
        name VARCHAR(50) NOT NULL,
        date_birth DATE NOT NULL,
        created_at DATE NOT NULL,
        updated_at DATE,
        deleted_at DATE,
        UNIQUE KEY uq_users_uuid (uuid),
        UNIQUE KEY uq_users_login (login)
    );

CREATE TABLE
    IF NOT EXISTS chat_types (
        id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
        name CHAR(36) NOT NULL
    );

INSERT INTO
    chat_types (name)
VALUES
    ('personal'),
    ('group');

CREATE TABLE
    IF NOT EXISTS chat (
        id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
        uuid CHAR(36) NOT NULL,
        name VARCHAR(100) NOT NULL,
        type_id INT NOT NULL,
        created_at DATE NOT NULL,
        updated_at DATE,
        deleted_at DATE
    );

CREATE TABLE
    IF NOT EXISTS messages (
        id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
        uuid CHAR(36) NOT NULL,
        chat_uuid CHAR(36) NOT NULL,
        message_text TEXT,
        created_at DATE NOT NULL,
        updated_at DATE,
        deleted_at DATE
    );

CREATE TABLE
    IF NOT EXISTS user_chats (
        id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
        user_uuid CHAR(36) NOT NULL,
        char_uuid CHAR(36) NOT NULL
    );