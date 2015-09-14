/**********************************************************************
File name: sig11.hpp
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
#ifndef SIG11_H
#define SIG11_H

#include <functional>
#include <map>
#include <mutex>
#include <vector>


namespace sig11 {

typedef uint64_t token_id;


class connection
{
public:
    connection(std::nullptr_t = nullptr);
    connection(const connection &ref) = delete;
    connection &operator=(const connection &ref) = delete;
    connection(connection &&src);
    connection &operator=(connection &&src);

protected:
    explicit connection(token_id id);

private:
    bool m_valid;
    token_id m_id;

public:
    inline operator bool() const
    {
        return m_valid;
    }

    inline token_id id() const
    {
        return m_id;
    }

    template <typename... ts> friend class signal;
    friend class testutils;
};

template <typename...>
class signal;

template <typename call_t>
class connection_guard;

template <typename result_t, typename... arg_ts>
class signal<result_t(arg_ts...)>
{
public:
    using call_t = result_t(arg_ts...);
    using function_type = std::function<call_t>;
    using guard_t = connection_guard<call_t>;

public:
    signal():
        m_token_id_ctr(0)
    {

    }

private:
    mutable std::mutex m_listeners_mutex;
    token_id m_token_id_ctr;
    std::map<token_id, function_type> m_listeners;

    std::vector<function_type*> m_listeners_tmp;

public:
    void operator()(const arg_ts&... args)
    {
        m_listeners_tmp.clear();
        {
            std::lock_guard<std::mutex> lock(m_listeners_mutex);
            for (auto &entry: m_listeners) {
                m_listeners_tmp.push_back(&entry.second);
            }
        }
        for (function_type *listener: m_listeners_tmp)
        {
            (*listener)(args...);
        }
    }

    connection connect(function_type &&receiver)
    {
        std::lock_guard<std::mutex> lock(m_listeners_mutex);
        token_id token = m_token_id_ctr++;
        m_listeners.emplace(token, std::move(receiver));
        return connection(token);
    }

    void disconnect(connection &conn)
    {
        if (!conn) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_listeners_mutex);
        auto iter = m_listeners.find(conn.id());
        if (iter == m_listeners.end()) {
            return;
        }
        m_listeners.erase(iter);
        conn = nullptr;
    }

};


template <typename call_t>
class connection_guard
{
public:
    using signal_t = signal<call_t>;

public:
    connection_guard(std::nullptr_t = nullptr):
        m_connection(nullptr),
        m_signal(nullptr)
    {

    }

    connection_guard(connection &&src, signal_t &signal):
        m_connection(std::move(src)),
        m_signal(&signal)
    {

    }

    connection_guard(const connection_guard &ref) = delete;
    connection_guard &operator=(const connection_guard &ref) = delete;
    connection_guard(connection_guard &&src):
        m_connection(std::move(src.m_connection)),
        m_signal(src.m_signal)
    {
        src.m_signal = nullptr;
    }

    connection_guard &operator=(connection_guard &&src)
    {
        disconnect();
        m_connection = std::move(src.m_connection);
        m_signal = src.m_signal;
        src.m_signal = nullptr;
        return *this;
    }

    connection_guard &operator=(std::nullptr_t)
    {
        disconnect();
        return *this;
    }

    ~connection_guard()
    {
        disconnect();
    }

private:
    connection m_connection;
    signal_t *m_signal;

public:
    inline operator bool() const
    {
        return bool(m_connection);
    }

    void disconnect()
    {
        if (m_signal) {
            m_signal->disconnect(m_connection);
            m_connection = nullptr;
            m_signal = nullptr;
        }
    }

};

template <typename call_t, typename callable_t>
static inline connection_guard<call_t> connect(signal<call_t> &signal,
                                               callable_t &&receiver)
{
    return connection_guard<call_t>(signal.connect(std::forward<callable_t>(receiver)), signal);
}

}

#endif
