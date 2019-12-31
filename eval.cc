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

#include <cstring>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>

#include <unistd.h>

#include "awd.hh"

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

void eval(std::vector<Token *> const &t) {
    for (auto token = std::begin(t); token != std::end(t); ++token) {
        if ((*token)->ty == TokenType::END) break;
        if (!std::strncmp(config_src + (*token)->start, "print",
                          (*token)->len)) {
            ++token;
            for (; (*token)->ty != TokenType::EOS;) {
                if ((*token)->ty == TokenType::IDENT) {
                    std::cerr << "Warning: Printing with identifier is not "
                                 "implemented."
                              << std::endl;
                } else if ((*token)->ty == TokenType::STR) {
                    std::cout.write((*token)->data, (*token)->data_len);
                    std::cout << std::endl;
                }
                ++token;
                if ((*token)->ty == TokenType::COMMA)
                    ++token;
                else if ((*token)->ty == TokenType::EOS) {
                    break;
                }
            }
        } else {
            diag("Undefined identifier.", (*token));
        }
    }
}
