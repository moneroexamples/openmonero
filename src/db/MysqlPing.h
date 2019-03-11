//
// Created by mwo on 12/07/18.
//

#ifndef OPENMONERO_MYSQLPING_H
#define OPENMONERO_MYSQLPING_H

#include "MySqlConnector.h"

#include <memory>
#include <thread>
#include <atomic>

namespace xmreg
{

using chrono::seconds;
using namespace literals::chrono_literals;

class MysqlPing
{
public:

    enum class StopReason {NotYetStopped, PingFailed, PointerExpired};

    MysqlPing(std::shared_ptr<MySqlConnector> _conn,
              seconds _ping_time = 300s,
              seconds _sleep_time = 5s);

    void operator()();
    void stop() {keep_looping = false;}

    uint64_t get_counter() const {return counter;}
    StopReason get_stop_reason() const {return why_stoped;};

    MysqlPing(MysqlPing&&) = default;
    MysqlPing& operator=(MysqlPing&&) = default;

    ~MysqlPing() = default;

private:
    std::weak_ptr<MySqlConnector> conn;
    seconds ping_time {300};
    seconds thread_sleep_time {5};
    atomic<bool> keep_looping {true};
    atomic<uint64_t> counter {0};
    atomic<StopReason> why_stoped {StopReason::NotYetStopped};
};
}

#endif //OPENMONERO_MYSQLPING_H
