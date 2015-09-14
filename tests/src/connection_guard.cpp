#include <catch.hpp>
#include "sig11/sig11.hpp"


void fun(int)
{

}


TEST_CASE("sig11/connection_guard/default_constructor")
{
    sig11::connection_guard<void(int)> guard;
    CHECK_FALSE(guard);
}

TEST_CASE("sig11/connection_guard/nullptr_constructor")
{
    sig11::connection_guard<void(int)> guard(nullptr);
    CHECK_FALSE(guard);
}

TEST_CASE("sig11/connection_guard/connection_constructor")
{
    sig11::signal<void(int)> signal;

    SECTION("with valid connection")
    {
        sig11::connection conn(signal.connect(&fun));
        CHECK(conn);
        sig11::connection_guard<void(int)> guard(std::move(conn), signal);
        CHECK(guard);
        CHECK_FALSE(conn);
    }
    SECTION("with invalid connection")
    {
        sig11::connection conn(nullptr);
        CHECK_FALSE(conn);
        sig11::connection_guard<void(int)> guard(std::move(conn), signal);
        CHECK_FALSE(guard);
    }
}

TEST_CASE("sig11/connection_guard/disconnect_in_destructor")
{
    sig11::signal<void(int)> signal;
    int destination = 0;

    auto fun = [&destination](int value){ destination = value; };

    {
        sig11::connection_guard<void(int)> guard(signal.connect(fun), signal);
        signal(10);
        CHECK(destination == 10);
        signal(20);
        CHECK(destination == 20);
    }
    signal(30);
    CHECK(destination == 20);
}

TEST_CASE("sig11/connection_guard/disconnect_on_nullptr_assignment")
{
    sig11::signal<void(int)> signal;
    int destination = 0;

    auto fun = [&destination](int value){ destination = value; };

    sig11::connection_guard<void(int)> guard(signal.connect(fun), signal);
    signal(10);
    CHECK(destination == 10);
    signal(20);
    CHECK(destination == 20);
    guard = nullptr;
    CHECK_FALSE(guard);

    signal(30);
    CHECK(destination == 20);
}

TEST_CASE("sig11/connection_guard/disconnect_on_other_assignment")
{
    sig11::signal<void(int)> signal;
    int destination1 = 0;
    int destination2 = 0;

    auto fun1 = [&destination1](int value){ destination1 = value; };
    auto fun2 = [&destination2](int value){ destination2 = value; };

    sig11::connection_guard<void(int)> guard1(signal.connect(fun1), signal);
    sig11::connection_guard<void(int)> guard2(signal.connect(fun2), signal);
    signal(10);
    CHECK(destination1 == 10);
    CHECK(destination2 == 10);
    signal(20);
    CHECK(destination1 == 20);
    CHECK(destination2 == 20);
    guard1 = std::move(guard2);
    CHECK_FALSE(guard2);

    signal(30);
    CHECK(destination1 == 20);
    CHECK(destination2 == 30);
}

TEST_CASE("sig11/connection_guard/disconnect()")
{
    sig11::signal<void(int)> signal;
    int destination = 0;

    auto fun = [&destination](int value){ destination = value; };


    sig11::connection_guard<void(int)> guard(signal.connect(fun), signal);
    signal(10);
    CHECK(destination == 10);
    signal(20);
    CHECK(destination == 20);
    guard.disconnect();
    CHECK_FALSE(guard);

    signal(30);
    CHECK(destination == 20);
}
