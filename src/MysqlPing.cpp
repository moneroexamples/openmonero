//
// Created by mwo on 12/07/18.
//

#include "MysqlPing.h"


namespace xmreg
{

MysqlPing::MysqlPing(
        std::shared_ptr<MySqlConnector> _conn,
        uint64_t _ping_time)
        : conn {_conn}, ping_time {_ping_time}
{}

void
MysqlPing::operator()()
{
    while (keep_looping)
    {
        std::this_thread::sleep_for(chrono::seconds(ping_time));

        if (auto c = conn.lock())
        {
            if (!c->ping())
            {
                cerr << "Pinging mysql failed. Stoping mysql pinging thread. \n";
                why_stoped = StopReason::PingFailed;
                break;
            }

            cout << "Mysql ping successful. \n" ;
        }
        else
        {
            cerr << "std::weak_ptr<MySqlConnector> conn expired! \n";
            why_stoped = StopReason::PointerExpired;
            break;
        }

        ++counter;
    }
}

}
