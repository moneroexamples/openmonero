//
// Created by mwo on 13/04/16.
//

#include "RPCCalls.h"

namespace xmreg
{

RPCCalls::RPCCalls(string _deamon_url, uint64_t _timeout)
    : deamon_url {_deamon_url}, timeout_time {_timeout}
{
    epee::net_utils::parse_url(deamon_url, url);

    port = std::to_string(url.port);

    timeout_time_ms = std::chrono::milliseconds {timeout_time};

    m_http_client.set_server(
            deamon_url,
            boost::optional<epee::net_utils::http::login>{});
}

RPCCalls::RPCCalls(RPCCalls&& a)
{
    std::lock_guard<std::mutex> guard(a.m_daemon_rpc_mutex);

    deamon_url = std::move(a.deamon_url);
    timeout_time = a.timeout_time;
    timeout_time_ms = a.timeout_time_ms;

    url = std::move(a.url);
    port = std::move(port);

    // we can't move or copy m_http_client,
    // so we just initialize it from zero
    m_http_client.set_server(
            deamon_url,
            boost::optional<epee::net_utils::http::login>{});

    // after the move, disconned the a object
    a.m_http_client.disconnect();

    cout << "\n RPCCalls(RPCCalls&& a) " << endl;
}

RPCCalls&
RPCCalls::operator=(RPCCalls&& a)
{
    if (*this == a)
        return *this;

    std::unique_lock<std::mutex> lhs_lk(m_daemon_rpc_mutex, std::defer_lock);
    std::unique_lock<std::mutex> rhs_lk(a.m_daemon_rpc_mutex, std::defer_lock);

    std::lock(lhs_lk, rhs_lk);

    deamon_url = std::move(a.deamon_url);
    timeout_time = a.timeout_time;
    timeout_time_ms = a.timeout_time_ms;

    url = std::move(a.url);
    port = std::move(port);

    // we can't move or copy m_http_client,
    // so we just initialize it from zero
    m_http_client.set_server(
            deamon_url,
            boost::optional<epee::net_utils::http::login>{});

    // after the move, disconned the a object
    a.m_http_client.disconnect();

    cout << "\n RPCCalls& operator=(RPCCalls&& a) " << endl;

    return *this;
}


bool
RPCCalls::operator==(RPCCalls const& a)
{
    return deamon_url == a.deamon_url;
}


bool
RPCCalls::operator!=(RPCCalls const& a)
{
    return !(*this == a);
}


bool
RPCCalls::connect_to_monero_deamon()
{
    if(m_http_client.is_connected())
    {
        return true;
    }

    return m_http_client.connect(timeout_time_ms);
}



bool
RPCCalls::commit_tx(
        const string& tx_blob,
        string& error_msg,
        bool do_not_relay)
{
    COMMAND_RPC_SEND_RAW_TX::request  req;
    COMMAND_RPC_SEND_RAW_TX::response res;

    req.tx_as_hex = tx_blob;

    req.do_not_relay = do_not_relay;

    std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

    if (!connect_to_monero_deamon())
    {
        cerr << "get_current_height: not connected to deamon" << endl;
        return false;
    }

    bool r = epee::net_utils::invoke_http_json(
            "/sendrawtransaction", req, res,
            m_http_client, timeout_time_ms);

    if (!r || res.status == "Failed")
    {
        error_msg = res.reason;

        if (error_msg.empty())
        {
            error_msg = "Reason not given by daemon.";
        }

        cerr << "Error sending tx: " << res.reason << endl;

        return false;
    }
    else if (res.status == "BUSY")
    {
        error_msg = "Deamon is BUSY. Cant sent now " + res.reason;

        cerr << "Error sending tx: " << error_msg << endl;

        return false;
    }

    if (do_not_relay)
    {
        cout << "Tx accepted by deamon but not relayed (useful for testing of constructing txs)" << endl;
    }

    return true;
}

bool
RPCCalls::commit_tx(
        tools::wallet2::pending_tx& ptx,
        string& error_msg)
{

    string tx_blob = epee::string_tools::buff_to_hex_nodelimer(
            tx_to_blob(ptx.tx)
    );

    return commit_tx(tx_blob, error_msg);
}


}
