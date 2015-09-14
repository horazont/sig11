/**********************************************************************
File name: first.cpp
This file is part of: sig11

LICENSE

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program.  If not, see <http://www.gnu.org/licenses/>.

FEEDBACK & QUESTIONS

For feedback and questions about sig11 please e-mail one of the authors named
in the AUTHORS file.
**********************************************************************/
#include <catch.hpp>

#include "sig11/sig11.hpp"

TEST_CASE("sig11/signal/connect_and_emit")
{
    sig11::signal<void(int)> signal;
    int destination = 0;

    auto fun = [&destination](int value){ destination = value; };

    signal.connect(fun);
    signal(10);
    CHECK(destination == 10);
    signal(20);
    CHECK(destination == 20);
}

TEST_CASE("sig11/signal/connect_emit_disconnect")
{
    sig11::signal<void(int)> signal;
    sig11::connection conn;
    CHECK_FALSE(conn);
    int destination = 0;

    auto fun = [&destination](int value){ destination = value; };

    conn = signal.connect(fun);
    CHECK(conn);
    signal(10);
    CHECK(destination == 10);
    signal(20);
    CHECK(destination == 20);
    signal.disconnect(conn);
    CHECK_FALSE(conn);
    signal(30);
    CHECK(destination == 20);
}

TEST_CASE("sig11/signal/connect_multiple")
{
    sig11::signal<void(int)> signal;
    sig11::connection conn;
    CHECK_FALSE(conn);
    std::vector<std::pair<int, int> > values;

    auto fun1 = [&values](int value){ values.emplace_back(0, value); };
    auto fun2 = [&values](int value){ values.emplace_back(1, value); };
    auto fun3 = [&values](int value){ values.emplace_back(2, value); };

    signal.connect(fun1);
    conn = signal.connect(fun2);
    signal.connect(fun3);
    CHECK(conn);
    signal(10);
    signal(20);
    signal.disconnect(conn);
    CHECK_FALSE(conn);
    signal(30);

    std::vector<std::pair<int, int> > reference({{0, 10}, {1, 10}, {2, 10}, {0, 20}, {1, 20}, {2, 20}, {0, 30}, {2, 30}});
    CHECK(values == reference);
}
