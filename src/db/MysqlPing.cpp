//
// Created by mwo on 12/07/18.
//

#include "easylogging++.h"
#include "../om_log.h"

#include "MysqlPing.h"


namespace xmreg
{

MysqlPing::MysqlPing(
        std::shared_ptr<MySqlConnector> _conn,
        seconds _ping_time)
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
                OMERROR << "Pinging mysql failed. Stoping mysql pinging thread.";
                why_stoped = StopReason::PingFailed;
                break;
            }

            OMINFO << "Mysql ping successful." ;
        }
        else
        {
            OMERROR << "std::weak_ptr<MySqlConnector> conn expired!";
            why_stoped = StopReason::PointerExpired;
            break;
        }

        ++counter;
    }

   OMINFO << "Exiting Mysql ping thread loop.";
}

}
