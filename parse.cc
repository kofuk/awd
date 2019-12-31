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

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

#include <unistd.h>

#include "awd.hh"

Token::Token()
    : start(0), len(0), line(0), col(0), data(nullptr), data_len(0) {}

Token::~Token() { delete[] data; }

/*
 * file = sentence (sentence)*
 * sentence = ident variable ("," variable)* ";"
 * variable = ident | string
 * string = "\"" .* "\""
 * ident = [a-zA-Z_][a-zA-Z0-9_]
 */

static std::size_t off;
static std::size_t line = 1;
static std::size_t col = 1;

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

static inline void forward() {
    if (config_src[off] == '\n' || config_src[off] == '\r') {
        ++line;
        col = 1;
    } else {
        ++col;
    }
    ++off;
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

static void parse_ident(std::vector<Token *> &t) {
    Token *result = new Token;
    result->ty = TokenType::IDENT;
    if (('a' <= config_src[off] && config_src[off] <= 'z') ||
        ('A' <= config_src[off] && config_src[off] <= 'Z') ||
        config_src[off] == '_') {
        result->start = off;
        result->line = line;
        result->col = col;
    } else
        diag("Expected identifier.");

    forward();

    while (('a' <= config_src[off] && config_src[off] <= 'z') ||
           ('A' <= config_src[off] && config_src[off] <= 'Z') ||
           config_src[off] == '_' ||
           ('0' <= config_src[off] && config_src[off] <= '9')) {
        forward();
    }
    result->len = off - result->start;
    t.push_back(result);
}

static void parse_string(std::vector<Token *> &t) {
    Token *result = new Token;
    result->ty = TokenType::STR;
    result->start = off;
    result->line = line;
    result->col = col;

    if (config_src[off] != '"') diag("Expected `\"'.");
    forward();

    while (config_src[off] != '"' && config_src[off] != 0) {
        forward();
    }

    if (config_src[off] != '"') diag("Missing terminating `\"'.");
    forward();

    result->len = off - result->start;
    if (result->len >= 2) {
        result->data = new char[result->len];
        std::copy(config_src + off - result->len + 1, config_src + off - 1,
                  result->data);
        result->data_len = result->len - 2;
    }

    t.push_back(result);
}

static void parse_variable(std::vector<Token *> &t) {
    if (config_src[off] == '"')
        parse_string(t);
    else
        parse_ident(t);
}

static void parse_comma(std::vector<Token *> &t) {
    Token *result = new Token;
    result->ty = TokenType::COMMA;
    result->start = off;
    result->line = line;
    result->col = col;
    if (config_src[off] != ',') diag("Expected comma.");
    forward();
    result->len = off - result->start;
    t.push_back(result);
}

static void parse_end_of_sentence(std::vector<Token *> &t) {
    Token *result = new Token;
    result->ty = TokenType::EOS;
    result->start = off;
    result->line = line;
    result->col = col;
    if (config_src[off] != ';') diag("Expected semicolon.");
    forward();
    result->len = off - result->start;
    t.push_back(result);
}

static void parse_sentence(std::vector<Token *> &t) {
    parse_ident(t);
    skip_space();
    if (config_src[off] == ';') {
        parse_end_of_sentence(t);
        return;
    }
    parse_variable(t);
    skip_space();
    do {
        if (config_src[off] == ',') {
            parse_comma(t);
        } else if (config_src[off] == ';') {
            parse_end_of_sentence(t);
            break;
        } else {
            diag("Expected comma or semicolon in this context.");
        }

        skip_space();
        parse_variable(t);
        skip_space();
    } while (true);
}

static void parse_file(std::vector<Token *> &t) {
    skip_space();
    for (; config_src[off];) {
        parse_sentence(t);
        skip_space();
    }

    Token *eof = new Token;
    eof->ty = TokenType::END;
    eof->start = off;
    eof->line = line;
    eof->col = col;
    eof->len = 0;
    t.push_back(eof);
}

std::vector<Token *> *parse() {
    std::vector<Token *> *result = new std::vector<Token *>();

    parse_file(*result);

    return result;
}
