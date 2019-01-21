//
// Created by mwo on 12/07/18.
//

#include "easylogging++.h"
#include "../om_log.h"

#include "MysqlPing.h"

#include <algorithm>


namespace xmreg
{

MysqlPing::MysqlPing(
        std::shared_ptr<MySqlConnector> _conn,
        seconds _ping_time, seconds _sleep_time)
        : conn {_conn}, ping_time {_ping_time},   
          thread_sleep_time {_sleep_time}  
{}

void
MysqlPing::operator()()
{

    // the while loop below is going to execute
    // faster than ping_time specified. The reason
    // is so that we can exit it in a timely manner
    // when keep_looping becomes false
    uint64_t between_ping_counter {0};
    
    // if ping_time lower than thread_sleep_time,
    // use ping_time for thread_sleep_time 
    thread_sleep_time = std::min(thread_sleep_time, ping_time);

    // we are going to ping mysql every
    // no_of_loops_between_pings iterations
    // of the while loop
    uint64_t no_of_loops_between_pings
            = std::max<uint64_t>(1, 
                    ping_time.count()/thread_sleep_time.count()) - 1;


    while (keep_looping)
    {
        std::this_thread::sleep_for(thread_sleep_time);
    
        if (++between_ping_counter <= no_of_loops_between_pings)
        {
            continue;
        }

        between_ping_counter = 0;

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
