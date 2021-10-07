//
// Created by mwo on 13/04/16.
//

#include "easylogging++.h"
#include "om_log.h"

#include "RPCCalls.h"

namespace xmreg
{

RPCCalls::RPCCalls(string _deamon_url, chrono::seconds _timeout)
    : deamon_url {_deamon_url}, rpc_timeout {_timeout}
{
    epee::net_utils::parse_url(deamon_url, url);

    port = std::to_string(url.port);

    m_http_client.set_server(
            deamon_url,
            boost::optional<epee::net_utils::http::login>{},
            epee::net_utils::ssl_support_t::e_ssl_support_disabled);
}

bool
RPCCalls::connect_to_monero_deamon()
{
    if(m_http_client.is_connected())
    {
        return true;
    }

    return m_http_client.connect(rpc_timeout);
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

    bool r {false};

    {
        std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

        r = epee::net_utils::invoke_http_json(
                "/sendrawtransaction", req, res,
                m_http_client, rpc_timeout);
    }

    if (!r || !check_if_response_is_ok(res, error_msg))
    {        
        if (res.reason.empty())
            error_msg += "Reason not given by daemon.";
        else
            error_msg += res.reason;

        OMERROR << "Error sending tx. " << error_msg;

        return false;
    }

    if (do_not_relay)
    {
        OMINFO << "Tx accepted by deamon but "
                  "not relayed (useful for testing "
                  "of constructing txs)";
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


bool
RPCCalls::get_current_height(uint64_t& current_height)
{
    COMMAND_RPC_GET_HEIGHT::request   req;
    COMMAND_RPC_GET_HEIGHT::response  res;

    bool r {false};

    {
        std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

        r = epee::net_utils::invoke_http_json(
                "/getheight",
                req, res, m_http_client, rpc_timeout);
    }

    string error_msg;

    if (!r || !check_if_response_is_ok(res, error_msg))
    {
        OMERROR << "Error getting blockchain height. "
                << error_msg;
        return false;
    }

    current_height = res.height;

    return true;
}

bool
RPCCalls::get_rct_output_distribution(std::vector<uint64_t>& rct_offsets)
{
    COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::request   req;
    COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::response  res;    

    req.amounts = {0}; // ringct outputs only
    req.cumulative = true;
    req.from_height = 0;
    req.to_height = 0;
    req.binary = true; // fastest request over local network

    bool r {false};

    {
        std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

        r = epee::net_utils::invoke_http_bin(
            "/get_output_distribution.bin",
            req, res, m_http_client, rpc_timeout);    
    }

    string error_msg;

    if (!r || !check_if_response_is_ok(res, error_msg))
    {
        OMERROR << "Error getting output distribution. " << error_msg;
        return false;
    }

   if (res.distributions.size() != 1)
        error_msg += "Unexpected size returned.";
    else if (res.distributions[0].amount != 0)
        error_msg += "Amount not 0.";

    if (!error_msg.empty())
    {
        OMERROR << "Error getting output distribution. " << error_msg;
        return false;
    }

    rct_offsets = res.distributions[0].data.distribution;

    return true;
}

template <typename Command>
bool
RPCCalls::check_if_response_is_ok(
        Command const& res,
        string& error_msg) const
{
    if (res.status == CORE_RPC_STATUS_BUSY)
    {
        error_msg = "Deamon is BUSY. Cant sent now. ";
        return false;
    }

    if (res.status != CORE_RPC_STATUS_OK)
    {
        error_msg = "RPC failed. ";
        return false;
    }

    return true;
}


template
bool RPCCalls::check_if_response_is_ok<
        COMMAND_RPC_SEND_RAW_TX::response>(
    COMMAND_RPC_SEND_RAW_TX::response const& res,
    string& error_msg) const;

template
bool RPCCalls::check_if_response_is_ok<
        COMMAND_RPC_GET_HEIGHT::response>(
    COMMAND_RPC_GET_HEIGHT::response const& res,
    string& error_msg) const;

template
bool RPCCalls::check_if_response_is_ok<
        COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::response>(
    COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::response const& res,
    string& error_msg) const;
}
