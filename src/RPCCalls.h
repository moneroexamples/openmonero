//
// Created by mwo on 13/04/16.
//


#ifndef CROWXMR_RPCCALLS_H
#define CROWXMR_RPCCALLS_H

#include "monero_headers.h"

#include <mutex>
#include <chrono>

namespace xmreg
{

using namespace cryptonote;
using namespace crypto;
using namespace std;

using namespace std::chrono_literals;

class RPCCalls
{
    string deamon_url;
    uint64_t timeout_time;

    //std::chrono::milliseconds timeout_time_ms;
    chrono::seconds rpc_timeout;

    epee::net_utils::http::url_content url;

    epee::net_utils::http::http_simple_client m_http_client;

    std::mutex m_daemon_rpc_mutex;

    string port;

public:

    RPCCalls(string _deamon_url = "http:://127.0.0.1:18081",
             chrono::seconds _timeout = 3min + 30s);    

    //RPCCalls(RPCCalls&& a);

    //RPCCalls&
    //operator=(RPCCalls&& a);

    //virtual bool
    //operator==(RPCCalls const& a);

    //virtual bool
    //operator!=(RPCCalls const& a);

    virtual bool
    connect_to_monero_deamon();

    virtual bool
    commit_tx(const string& tx_blob,
              string& error_msg,
              bool do_not_relay = false);

    virtual bool
    commit_tx(tools::wallet2::pending_tx& ptx,
              string& error_msg);


    virtual ~RPCCalls() = default;
};


}

#endif //CROWXMR_RPCCALLS_H
