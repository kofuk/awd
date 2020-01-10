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
#include <stack>
#include <string>
#include <vector>

#include <unistd.h>

#include "awd.hh"

AwdVariable::AwdVariable() : tk(nullptr), ref_count(0) {}

AwdVariable::~AwdVariable() {
    if (type == VarType::REF) {
        delete val.ref_name;
    } else if (type == VarType::STR) {
        delete val.str_val;
    }
}

static void diag(std::string const &msg, Token const *t) {
    std::cerr << progname << ": error: " << t->line << ": " << t->col << ": "
              << msg << std::endl;

    if (t == nullptr) {
        std::cerr << "Abort." << std::endl;

        return;
    }

    std::cerr << std::setw(5) << t->line << " | ";

    std::size_t beg;
    std::size_t end;
    for (beg = t->start; beg != 0 && config_src[beg] != '\n'; --beg)
        ;
    if (config_src[beg] == '\n') ++beg;
    for (end = beg; config_src[end] != '\n' && config_src[end] != 0; ++end)
        ;
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

static std::stack<AwdVariable *> stack;

AwdVariable *operation_pop() {
    AwdVariable *top = stack.top();
    stack.pop();

    return top;
}

void operation_push(AwdVariable &v) { stack.push(&v); }

int operation_halt() {
    AwdVariable *arg = operation_pop();
    if (arg->type != VarType::INT)
        diag("Mismatched type (expected int value.)", arg->tk);
    int val = arg->val.int_val;
    --arg->ref_count;
    if (arg->ref_count < 1) delete arg;

    return val;
}

int eval(std::vector<Text *> const &executable) {
    for (auto text = std::begin(executable); text != std::end(executable);
         ++text) {
        switch ((*text)->op) {
        case Operation::PUSH:
            operation_push(*(*text)->arg);
            break;
        case Operation::HALT:
            return operation_halt();
            break;
        }
    }

    /* Should not reach. */
    return 1;
}
