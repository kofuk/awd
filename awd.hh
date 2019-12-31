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

#include <string>
#include <vector>

static std::string progname = "awd";

enum class TokenType { IDENT, STR, COMMA, EOS, END };

struct Token {
    Token();
    ~Token();

    TokenType ty;
    std::size_t start;
    std::ptrdiff_t len;
    std::size_t line;
    std::size_t col;
    char *data;
    std::size_t data_len;
};

extern char *config_src;

std::vector<Token *> *parse();
void eval(std::vector<Token *> const &);

#endif
