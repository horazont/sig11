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
#include <utility>
#include <vector>


namespace sig11 {

typedef uint64_t token_id;

/**
 * The connection class represents a %connection between a signal and a
 * receiver.
 *
 * By itself, the connection implements no management. If you want to have the
 * connection to be disconnected automatically when it leaves the scope,
 * you should use connection_guard.
 *
 * @see signal::connect
 * @see connection_guard
 * @see connect
 */
class connection
{
public:
    /**
     * Construct an invalid (empty) connection.
     *
     * Such a connection compares equal to nullptr.
     */
    explicit connection(std::nullptr_t = nullptr);

    connection(const connection &ref) = delete;
    connection &operator=(const connection &ref) = delete;

    /**
     * Move a connection into a new one.
     */
    connection(connection &&src);

    /**
     * Move another connection into this one.
     */
    connection &operator=(connection &&src);

    /**
     * Clear this connection. A cleared connection is equivalent to a connection
     * created from nullptr.
     */
    connection &operator=(std::nullptr_t);

protected:
    /**
     * Construct a valid connection with the given token id.
     */
    explicit connection(token_id id);

private:
    bool m_valid;
    token_id m_id;

public:
    /**
     * Return true if the connection is valid.
     */
    inline operator bool() const
    {
        return m_valid;
    }

    /**
     * The token ID of the connection. If the connection is not valid the
     * result is unspecified.
     */
    inline token_id id() const
    {
        return m_id;
    }

    template <typename T> friend class signal;
    friend class testutils;

    friend void swap(connection &a, connection &b);
};

/**
 * Swap two connections in-place.
 */
void swap(connection &a, connection &b);

static inline bool operator==(std::nullptr_t, const connection &conn)
{
    return !conn;
}

static inline bool operator!=(std::nullptr_t, const connection &conn)
{
    return bool(conn);
}

static inline bool operator==(const connection &conn, std::nullptr_t)
{
    return !conn;
}

static inline bool operator!=(const connection &conn, std::nullptr_t)
{
    return bool(conn);
}


template <typename T>
class signal;

template <typename call_t>
class connection_guard;

/**
 * The signal template allows to define signals.
 *
 * Signals are callables. A signal is defined for a single function call
 * signature, and signals with return values are not supported.
 *
 * All operations on a signal are thread-safe with respect to each other, with
 * one notable exception: it is not safe to emit the signal from multiple
 * threads without synchronization.
 *
 * The argument types of a signal must be copyable.
 */
template <typename result_t, typename... arg_ts>
class signal<result_t(arg_ts...)>
{
public:
    static_assert(std::is_void<result_t>::value,
                  "signals with non-void return values are not supported.");

    using call_t = result_t(arg_ts...);
    using function_type = std::function<call_t>;
    using guard_t = connection_guard<call_t>;

public:
    /**
     * Construct a new signal without any connected receivers.
     */
    signal():
        m_token_id_ctr(0)
    {

    }

    signal(const signal &ref) = delete;
    signal &operator=(const signal &ref) = delete;
    signal(signal &&src) = delete;
    signal &operator=(signal &&src) = delete;

private:
    mutable std::mutex m_listeners_mutex;
    token_id m_token_id_ctr;
    std::map<token_id, function_type> m_listeners;

    std::vector<function_type*> m_listeners_tmp;

public:
    /**
     * Emit the signal with the given arguments.
     *
     * The arguments are restricted to be const references as they are copied
     * for each receiver.
     *
     * No two threads must call this function without synchronization.
     */
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

    /**
     * Connect a \a receiver to the signal.
     *
     * This function is thread-safe.
     *
     * @param receiver The std::function object to connect.
     * @return A connection for the newly connected receiver.
     * @see disconnect()
     * @see sig11::connect()
     */
    connection connect(function_type &&receiver)
    {
        std::lock_guard<std::mutex> lock(m_listeners_mutex);
        token_id token = m_token_id_ctr++;
        m_listeners.emplace(token, std::move(receiver));
        return connection(token);
    }

    /**
     * Disconnect a given connection \a conn from the signal.
     *
     * If the \a conn is not valid or refers to a non-existent connection, this
     * is a no-op.
     *
     * This function is thread-safe.
     *
     * @param conn The connection to disconnect.
     */
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


/**
 * Use connection_guard to implement scoped disconnection of connections.
 *
 * A connection_guard takes a connection and a signal. When it goes out of
 * scope or is assigned another value (including nullptr), the connection is
 * disconnected from the signal.
 */
template <typename call_t>
class connection_guard
{
public:
    using signal_t = signal<call_t>;

public:
    /**
     * Create an invalid connection_guard which is not bound to a connection.
     */
    connection_guard(std::nullptr_t = nullptr):
        m_connection(nullptr),
        m_signal(nullptr)
    {

    }

    /**
     * Create a connection_guard which is bound to the connection \a conn and
     * the signal \a signal.
     *
     * The use of sig11::connect over this constructor is preferred.
     *
     * @param conn The connection to manage.
     * @param signal The signal to which the connection belongs.
     */
    connection_guard(connection &&conn, signal_t &signal):
        m_connection(std::move(conn)),
        m_signal(&signal)
    {

    }

    connection_guard(const connection_guard &ref) = delete;
    connection_guard &operator=(const connection_guard &ref) = delete;

    /**
     * Move another connection_guard into a new one.
     *
     * The \a src is equivalent to a connection_guard initialized with the
     * default constructor (or nullptr).
     *
     * @param src The connection_guard to move.
     */
    connection_guard(connection_guard &&src):
        m_connection(std::move(src.m_connection)),
        m_signal(src.m_signal)
    {
        src.m_signal = nullptr;
    }

    /**
     * Move another connection_guard into this one.
     *
     * If the guard currently holds a connection, it is disconnected before
     * moving.
     *
     * The \a src is equivalent to a connection_guard initialized with the
     * default constructor (or nullptr).
     *
     * @param src The connection_guard to move.
     */
    connection_guard &operator=(connection_guard &&src)
    {
        disconnect();
        m_connection = std::move(src.m_connection);
        m_signal = src.m_signal;
        src.m_signal = nullptr;
        return *this;
    }

    /**
     * Clear this connection_guard.
     *
     * This disconnects the connection held by the guard, if any.
     */
    connection_guard &operator=(std::nullptr_t)
    {
        disconnect();
        return *this;
    }

    /**
     * Destruct the connection_guard and disconnect any connection currently
     * held by it.
     */
    ~connection_guard()
    {
        disconnect();
    }

private:
    connection m_connection;
    signal_t *m_signal;

public:
    /**
     * Return true if the guard holds a connection.
     */
    inline operator bool() const
    {
        return bool(m_connection);
    }

    /**
     * Disconnect the connection held by the guard, if any.
     *
     * This is equivalent to assigning a nullptr.
     */
    void disconnect()
    {
        if (m_signal) {
            m_signal->disconnect(m_connection);
            release();
        }
    }

    /**
     * Clear the connection guard without disconnecting the connection held by
     * it.
     */
    void release()
    {
        if (m_signal) {
            m_connection = nullptr;
            m_signal = nullptr;
        }
    }


    template <typename T> friend void swap(connection_guard<T> &a, connection_guard<T> &b);

};


template <typename call_t>
static inline bool operator==(std::nullptr_t, const connection_guard<call_t> &guard)
{
    return !guard;
}

template <typename call_t>
static inline bool operator!=(std::nullptr_t, const connection_guard<call_t> &guard)
{
    return bool(guard);
}

template <typename call_t>
static inline bool operator==(const connection_guard<call_t> &guard, std::nullptr_t)
{
    return !guard;
}

template <typename call_t>
static inline bool operator!=(const connection_guard<call_t> &guard, std::nullptr_t)
{
    return bool(guard);
}


/**
 * Swap two connection_guards in-place.
 */
template <typename call_t>
static inline void swap(connection_guard<call_t> &a, connection_guard<call_t> &b)
{
    swap(a.m_connection, b.m_connection);
    std::swap(a.m_signal, b.m_signal);
}


/**
 * This is a handy template to connect a \a receiver to a \a signal.
 *
 * It returns a connection_guard for the new connection.
 */
template <typename call_t, typename callable_t>
static inline connection_guard<call_t> connect [[gnu::warn_unused_result]] (signal<call_t> &signal,
                                                                            callable_t &&receiver)
{
    return connection_guard<call_t>(signal.connect(std::forward<callable_t>(receiver)), signal);
}


}

#endif
