#include <catch.hpp>
#include "sig11/sig11.hpp"


namespace sig11 {

class testutils
{
public:
    static inline connection make_conn(token_id token)
    {
        return connection(token);
    }
};

}


TEST_CASE("sig11/connection/default_constructor")
{
    sig11::connection conn;
    CHECK_FALSE(conn);
}

TEST_CASE("sig11/connection/token_id_constructor")
{
    CHECK(sig11::testutils::make_conn(10).id() == 10);
    CHECK(sig11::testutils::make_conn(10));
}

TEST_CASE("sig11/connection/nullptr_constructor")
{
    sig11::connection conn(nullptr);
    CHECK_FALSE(conn);
}

TEST_CASE("sig11/connection/move_constructor")
{
    SECTION("from new")
    {
        sig11::connection conn(sig11::testutils::make_conn(10));
        CHECK(conn);
        CHECK(conn.id() == 10);
    }
    SECTION("from existing")
    {
        sig11::connection conn1(sig11::testutils::make_conn(10));
        sig11::connection conn2(std::move(conn1));
        CHECK(conn2);
        CHECK(conn2.id() == 10);
        CHECK_FALSE(conn1);
    }
}

TEST_CASE("sig11/connection/move_assigment")
{
    SECTION("with valued peer")
    {
        sig11::connection conn1(sig11::testutils::make_conn(10));
        sig11::connection conn2(sig11::testutils::make_conn(20));

        conn1 = std::move(conn2);
        CHECK(conn1.id() == 20);
        CHECK(conn1);
        CHECK_FALSE(conn2);
    }
    SECTION("with nullptr peer")
    {
        sig11::connection conn1(sig11::testutils::make_conn(10));
        sig11::connection conn2(nullptr);

        conn1 = std::move(conn2);
        CHECK_FALSE(conn1);
        CHECK_FALSE(conn2);
    }
    SECTION("to nullptr with valued peer")
    {
        sig11::connection conn1(nullptr);
        sig11::connection conn2(sig11::testutils::make_conn(20));

        conn1 = std::move(conn2);
        CHECK(conn1);
        CHECK(conn1.id() == 20);
        CHECK_FALSE(conn2);
    }
}

TEST_CASE("sig11/connection/nullptr_assignment")
{
    sig11::connection conn(sig11::testutils::make_conn(10));
    conn = nullptr;
    CHECK_FALSE(conn);
}
