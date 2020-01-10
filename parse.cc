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
#include <cassert>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <unistd.h>

#include "awd.hh"

static std::size_t off;

static std::vector<Token *> tk;

static void diag(std::string const &msg, Token const *t) {
    std::cerr << progname << ": error: " << t->line << ": " << t->col << ": "
              << msg << std::endl;

    std::cerr << std::setw(5) << t->line << " | ";

    std::size_t beg;
    std::size_t end;
    for (beg = t->start; beg != 0 && config_src[beg] != '\n'; --beg) {
    }
    if (config_src[beg] == '\n') ++beg;
    for (end = beg; config_src[end] != '\n' && config_src[end] != 0; ++end) {
    }
    std::cerr.write(config_src + beg, end - beg);
    std::cerr << std::endl;

    for (std::size_t i = 0; i < t->col + 7; ++i) {
        std::cerr << " ";
    }

    std::cerr << "^";
    if (t->len > 0) {
        for (std::size_t i = 0; i < (std::size_t)t->len - 1; ++i) {
            std::cerr << "~";
        }
    }
    std::cerr << std::endl;

    std::cerr << "Abort." << std::endl;

    _exit(1);
}

static bool forward() {
    if (tk[off]->ty != TokenType::END_FILE) {
        ++off;
        return true;
    } else {
        return false;
    }
}

/*
 * ident = [a-zA-Z_][a-zA-Z0-9_]
 */
static void parse_ident(std::vector<Text *> &t) {
    AwdVariable *v = new AwdVariable;
    v->type = VarType::REF;
    v->ref_count = 1;
    Token *current_token = tk[off];
    std::string *name =
        new std::string(config_src[current_token->start], current_token->len);
    v->val.ref_name = name;

    Text *text = new Text;
    text->op = Operation::PUSH;
    text->arg = v;
    text->tk = current_token;

    t.push_back(text);

    forward();
}

/*
 * string-literal = "\"" .* "\""
 */
static void parse_string(std::vector<Text *> &t) {}

/*
 * integer-literal = [0-9]+
 */
static void parse_int(std::vector<Text *> &t) {
    AwdVariable *v = new AwdVariable;
    v->type = VarType::INT;
    v->ref_count = 1;
    Token *current_token = tk[off];
    v->val.int_val = 0;
    for (std::size_t i = 0; i < (std::size_t)current_token->len; ++i) {
        assert('0' <= config_src[current_token->start + i] &&
               config_src[current_token->start + i] <= '9');
        v->val.int_val *= 10;
        v->val.int_val += config_src[current_token->start + i] - '0';
    }
    Text *text = new Text;
    text->op = Operation::PUSH;
    text->tk = current_token;
    text->arg = v;

    t.push_back(text);

    forward();
}

/*
 * variable = ident | string-literal | integer-literal
 */
static void parse_variable(std::vector<Text *> &t) {
    if (tk[off]->ty == TokenType::IDENT) {
        parse_ident(t);
    } else if (tk[off]->ty == TokenType::STR) {
        parse_string(t);
    } else if (tk[off]->ty == TokenType::INT) {
        parse_int(t);
    } else {
        diag("Expect identifier or string literal.", tk[off]);
    }
}

/*
 * sentence = ((ident (variable ("," variable)*)?)? | "exit") ";"
 */
static void parse_sentence(std::vector<Text *> &t) {
    if (tk[off]->ty == TokenType::IDENT) {
        std::size_t ident_off = off;
        forward();
        while (tk[off]->ty != TokenType::END_SENTENCE) {
            parse_variable(t);
            if (tk[off]->ty == TokenType::END_SENTENCE)
                break;
            else if (tk[off]->ty != TokenType::COMMA) {
                diag("Expect `,' or `;'.", tk[off]);
            }
        }
        forward();

        std::size_t new_off = off;
        off = ident_off;
        parse_ident(t);
        off = new_off;

    } else if (tk[off]->ty == TokenType::EXIT) {
        Text *op = new Text;
        op->op = Operation::HALT;
        op->tk = tk[off];
        forward();
        if (tk[off]->ty != TokenType::INT) {
            diag("Expect integer literal.", tk[off]);
        }

        parse_int(t);

        t.push_back(op);

        if (tk[off]->ty != TokenType::END_SENTENCE) {
            diag("Expected just 1 argument.", tk[off]);
        }
        forward();
    } else if (tk[off]->ty != TokenType::END_SENTENCE) {
        diag("Expect identifier.", tk[off]);
    }
}

/*
 * program = (sentence)*
 */
static void parse_program(std::vector<Text *> &t) {
    while (tk[off]->ty != TokenType::END_FILE) {
        parse_sentence(t);
    }

    assert(tk[off]->ty == TokenType::END_FILE);

    AwdVariable *v = new AwdVariable;
    v->type = VarType::INT;
    v->val.int_val = 0;

    Text *push_retval = new Text;
    push_retval->op = Operation::PUSH;
    push_retval->arg = v;
    push_retval->tk = tk[off];
    t.push_back(push_retval);

    Text *op = new Text;
    op->op = Operation::HALT;
    op->tk = tk[off];
    t.push_back(op);
}

std::vector<Text *> *parse(std::vector<Token *> const &token) {
    off = 0;

    tk = token;
    std::vector<Text *> *executable = new std::vector<Text *>;

    parse_program(*executable);

    return executable;
}
