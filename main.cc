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

#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "awd.hh"

char *config_src;

int main() {
    struct stat st;
    if (stat("Awdfile", &st) == -1) {
        std::perror(progname.c_str());

        return 1;
    }

    if ((st.st_mode & S_IFMT) != S_IFREG) {
        std::perror(progname.c_str());

        return 1;
    }

    std::ifstream in("Awdfile");
    if (!in) {
        std::perror(progname.c_str());

        return 1;
    }
    config_src = new char[st.st_blksize + 1];
    in.read(config_src, st.st_blksize);
    config_src[st.st_blksize] = 0;

    std::vector<Token *> *tokens = tokenize();
    std::vector<Text *> *executable = parse(*tokens);
    int result = eval(*executable);

    for (auto itr = std::begin(*executable); itr != std::end(*executable); ++itr)
        delete *itr;
    delete executable;

    for (auto itr = std::begin(*tokens); itr != std::end(*tokens); ++itr)
        delete *itr;
    delete tokens;
    delete[] config_src;

    return result;
}
