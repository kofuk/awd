/*
 * Copyright (C) 2019  Koki Fukuda
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <iomanip>
#include <iostream>

#include <unistd.h>

#include "awd.hh"

static std::size_t off;
static std::size_t line;
static std::size_t col;

static void diag(std::string const &msg) {
    std::cerr << progname << ": error: " << line << ": " << col << ": " << msg
              << std::endl;

    std::cerr << std::setw(5) << line << " | ";

    std::size_t beg;
    std::size_t end;
    for (beg = off; beg != 0 && config_src[beg] != '\n'; --beg) {
    }
    if (config_src[beg] == '\n') ++beg;
    for (end = beg; config_src[end] != '\n' && config_src[end] != 0; ++end) {
    }
    std::cerr.write(config_src + beg, end - beg);
    std::cerr << std::endl;

    for (std::size_t i = 0; i < col + 7; ++i) {
        std::cerr << " ";
    }

    std::cerr << "^" << std::endl;

    std::cerr << "Abort." << std::endl;

    _exit(1);
}

static inline bool forward() {
    if (config_src[off] == 0) return false;
    if (config_src[off] == '\n' || config_src[off] == '\r') {
        ++line;
        col = 1;
    } else {
        ++col;
    }
    ++off;

    return true;
}

static inline void skip_space() {
    for (;;) {
        while (config_src[off] == ' ' || config_src[off] == '\t' ||
               config_src[off] == '\n' || config_src[off] == '\r')
            forward();
        if (config_src[off] == '#') {
            forward();
            while (config_src[off] != '\n')
                forward();
        } else {
            break;
        }
    }
}

struct Keywords {
    std::string kw;
    TokenType ty;
};

Keywords keywords[] = {{"exit", TokenType::EXIT}};

static bool tokenize_keyword(std::vector<Token *> &t) {
    for (auto itr = std::begin(keywords); itr != std::end(keywords); ++itr) {
        if (!((*itr).kw.compare(0, std::string::npos, config_src + off,(*itr).kw.size()))) {
            char next = config_src[off + (*itr).kw.size()];
            if ((next < 'a' || 'z' < next) && (next < 'A' || 'Z' < next) &&
                (next < '0' || '9' < next) && next != '_') {
                Token *result = new Token;
                result->ty = (*itr).ty;
                result->line = line;
                result->col = col;
                result->start = off;
                result->len = (*itr).kw.size();

                for (std::size_t i = (*itr).kw.size(); i != 0; --i)
                    forward();

                t.push_back(result);

                return true;
            }
        }
    }

    return false;
}

static bool tokenize_punctuator(std::vector<Token *> &t) {
    if (config_src[off] == ';') {
        Token *result = new Token;
        result->ty = TokenType::END_SENTENCE;
        result->start = off;
        result->line = line;
        result->col = col;
        result->len = 1;
        t.push_back(result);

        forward();

        return true;
    } else if (config_src[off] == ',') {
        Token *result = new Token;
        result->ty = TokenType::COMMA;
        result->start = off;
        result->line = line;
        result->col = col;
        result->len = 1;
        t.push_back(result);

        forward();

        return true;
    }

    return false;
}

static bool tokenize_number(std::vector<Token *> &t) {
    if (config_src[off] < '0' || '9' < config_src[off]) return false;

    Token *result = new Token;
    result->ty = TokenType::INT;
    result->start = off;
    result->line = line;
    result->col = col;

    forward();

    while ('0' <= config_src[off] && config_src[off] <= '9')
        forward();

    result->len = off - result->start;

    t.push_back(result);

    return true;
}

static bool tokenize_string_literal(std::vector<Token *> &t) {
    if (config_src[off] != '"') {
        return false;
    }

    Token *result = new Token;
    result->ty = TokenType::STR;
    result->start = off;
    result->line = line;
    result->col = col;

    forward();
    while (config_src[off] != '"') {
        if (!forward()) {
            diag("Unterminated string literal.");
        }
    }
    forward();

    result->len = off - result->start;

    t.push_back(result);

    return true;
}

static bool tokenize_identifier(std::vector<Token *> &t) {
    if ((config_src[off] < 'a' || 'z' < config_src[off]) &&
        (config_src[off] < 'A' || 'Z' < config_src[off]) &&
        config_src[off] != '_') {
        return false;
    }

    Token *result = new Token;
    result->ty = TokenType::IDENT;
    result->start = off;
    result->line = line;
    result->col = col;

    forward();

    while (('a' <= config_src[off] && config_src[off] <= 'z') ||
           ('A' <= config_src[off] && config_src[off] <= 'Z') ||
           ('0' <= config_src[off] && config_src[off] <= '9') ||
           config_src[off] == '_') {
        forward();
    }
    result->len = off - result->start;

    t.push_back(result);

    return true;
}

std::vector<Token *> *tokenize() {
    off = 0;
    line = 1;
    col = 1;

    std::vector<Token *> *result = new std::vector<Token *>;

    for (;;) {
        skip_space();
        if (!config_src[off]) break;

        if (tokenize_punctuator(*result)) continue;
        if (tokenize_keyword(*result)) continue;
        if (tokenize_string_literal(*result)) continue;
        if (tokenize_number(*result)) continue;
        if (tokenize_identifier(*result)) continue;

        diag("Unrecognized token.");
    }

    Token *eof = new Token;
    eof->ty = TokenType::END_FILE;
    eof->start = off;
    eof->len = 0;
    eof->line = line;
    eof->col = col;
    result->push_back(eof);

    return result;
}
