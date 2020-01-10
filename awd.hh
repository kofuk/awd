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

#ifndef AWD_HH
#define AWD_HH

#include <set>
#include <string>
#include <vector>

static std::string progname = "awd";

enum class TokenType { IDENT, STR, INT, COMMA, EXIT, END_SENTENCE, END_FILE };

struct Token {
    TokenType ty;
    std::size_t start;
    std::ptrdiff_t len;
    std::size_t line;
    std::size_t col;
};

enum class VarType { REF, INT, STR };

struct AwdVariable {
    union Value {
        std::string *ref_name;
        int int_val;
        std::string *str_val;
    };

    VarType type;
    Value val;
    Token *tk;

    int ref_count;

    AwdVariable();
    ~AwdVariable();
};

enum class Operation { HALT, PUSH };

struct Text {
    Operation op;
    Token *tk;
    AwdVariable *arg;
};

extern char *config_src;

std::vector<Token *> *tokenize();
std::vector<Text *> *parse(std::vector<Token *> const &);
int eval(std::vector<Text *> const &executable);

#endif
