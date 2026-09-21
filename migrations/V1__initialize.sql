CREATE TABLE
    IF NOT EXISTS users (
        id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
        uuid CHAR(36) NOT NULL,
        login VARCHAR(255) NOT NULL,
        password_hash VARCHAR(255) NOT NULL,
        name VARCHAR(50) NOT NULL,
        date_birth DATE NOT NULL,
        PRIMARY KEY (id),
        UNIQUE KEY uq_users_uuid (uuid),
        UNIQUE KEY uq_users_login (login)
    );