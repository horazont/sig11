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

#include <condition_variable>
#include <thread>


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

TEST_CASE("sig11/signal/disconnect_during_emit")
{
    sig11::signal<void(int)> signal;
    sig11::connection conn;
    CHECK_FALSE(conn);
    int destination = 0;

    auto fun = [&destination, &conn, &signal](int value){ destination = value; signal.disconnect(conn); };

    conn = signal.connect(fun);
    CHECK(conn);
    signal(10);
    CHECK(destination == 10);
    CHECK_FALSE(conn);
    signal(20);
    CHECK(destination == 10);
}


class ThreadTesterConnectEmit
{
public:
    ThreadTesterConnectEmit(sig11::signal<void(int)> &signal, std::function<void(int)> &&to_connect):
        m_signal(signal),
        m_to_connect(std::move(to_connect)),
        m_doit(false),
        m_done(false)
    {

    }

private:
    sig11::signal<void(int)> &m_signal;
    std::function<void(int)> m_to_connect;

    std::mutex m_notify_mutex;
    std::condition_variable m_doit_notify;
    bool m_doit;

    std::mutex m_done_mutex;
    std::condition_variable m_done_notify;
    bool m_done;

public:
    void thread()
    {
        {
            std::unique_lock<std::mutex> lock(m_notify_mutex);
            m_doit_notify.wait(lock, [this](){return m_doit;});
            m_doit = false;
        }
        m_signal.connect(std::move(m_to_connect));
        {
            std::lock_guard<std::mutex> lock(m_done_mutex);
            m_done = true;
        }
        m_done_notify.notify_all();
    }

    void notify_and_wait_for_done()
    {
        {
            std::lock_guard<std::mutex> lock(m_notify_mutex);
            m_doit = true;
        }
        m_doit_notify.notify_all();
        {
            std::unique_lock<std::mutex> lock(m_done_mutex);
            m_done_notify.wait(lock, [this](){return m_done; });
            m_done = false;
        }
    }

};


TEST_CASE("sig11/signal/thread_safety_connect_vs_emit")
{
    sig11::signal<void(int)> signal;

    std::vector<std::pair<int, int>> destination;

    auto fun = [&destination](int value){ destination.emplace_back(2, value); };
    ThreadTesterConnectEmit tester(signal, fun);

    auto sync_with_thread = [&tester, &destination](int value){ destination.emplace_back(0, value); tester.notify_and_wait_for_done(); destination.emplace_back(1, value); };

    std::thread thread(std::bind(&ThreadTesterConnectEmit::thread, &tester));

    sig11::connection conn = signal.connect(sync_with_thread);
    signal(10);
    signal.disconnect(conn);
    signal(20);

    std::vector<std::pair<int, int>> reference({{0, 10}, {1, 10}, {2, 20}});
    CHECK(destination == reference);

    thread.join();
}


class ThreadTesterDisconnectEmit
{
public:
    ThreadTesterDisconnectEmit(sig11::signal<void(int)> &signal, sig11::connection &&to_disconnect):
        m_signal(signal),
        m_to_disconnect(std::move(to_disconnect)),
        m_doit(false),
        m_done(false)
    {

    }

private:
    sig11::signal<void(int)> &m_signal;
    sig11::connection m_to_disconnect;

    std::mutex m_notify_mutex;
    std::condition_variable m_doit_notify;
    bool m_doit;

    std::mutex m_done_mutex;
    std::condition_variable m_done_notify;
    bool m_done;

public:
    void thread()
    {
        {
            std::unique_lock<std::mutex> lock(m_notify_mutex);
            m_doit_notify.wait(lock, [this](){return m_doit;});
            m_doit = false;
        }
        m_signal.disconnect(m_to_disconnect);
        {
            std::lock_guard<std::mutex> lock(m_done_mutex);
            m_done = true;
        }
        m_done_notify.notify_all();
    }

    void notify_and_wait_for_done()
    {
        {
            std::lock_guard<std::mutex> lock(m_notify_mutex);
            m_doit = true;
        }
        m_doit_notify.notify_all();
        {
            std::unique_lock<std::mutex> lock(m_done_mutex);
            m_done_notify.wait(lock, [this](){return m_done; });
            m_done = false;
        }
    }

};


TEST_CASE("sig11/signal/thread_safety_disconnect_vs_emit")
{
    sig11::signal<void(int)> signal;

    std::vector<std::pair<int, int>> destination;

    auto fun = [&destination](int value){ destination.emplace_back(2, value); };
    sig11::connection conn_to_disconnect(signal.connect(fun));

    ThreadTesterDisconnectEmit tester(signal, std::move(conn_to_disconnect));

    auto sync_with_thread = [&tester, &destination](int value){ destination.emplace_back(0, value); tester.notify_and_wait_for_done(); destination.emplace_back(1, value); };

    std::thread thread(std::bind(&ThreadTesterDisconnectEmit::thread, &tester));

    sig11::connection conn = signal.connect(sync_with_thread);
    signal(10);
    signal.disconnect(conn);
    signal(20);

    std::vector<std::pair<int, int>> reference({{2, 10}, {0, 10}, {1, 10}});
    CHECK(destination == reference);

    thread.join();
}


TEST_CASE("sig11/connect")
{
    sig11::signal<void(int)> signal;
    int destination = 0;

    auto fun = [&destination](int value){ destination = value; };

    sig11::connect(signal, fun);
    signal(10);
    CHECK(destination == 0);

    auto guard(sig11::connect(signal, fun));
    signal(10);
    CHECK(destination == 10);
}
