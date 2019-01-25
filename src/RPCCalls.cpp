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
            boost::optional<epee::net_utils::http::login>{});
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
       // std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

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
RPCCalls::get_blocks(vector<uint64_t> blk_heights,
              COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::response& res)
{
    COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::request req;
    req.heights = std::move(blk_heights);

    bool r {false};

    {
        std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

        r = epee::net_utils::invoke_http_bin(
                "/get_blocks_by_height.bin",
                req, res, m_http_client, rpc_timeout);
    }

    string error_msg;

    if (!r || !check_if_response_is_ok(res, error_msg))
    {
        OMERROR << "Error getting blocks. "
                << error_msg;
        return false;
    }

    return true;
}

std::vector<pair<block, vector<transaction>>>
RPCCalls::get_blocks_range(uint64_t h1, uint64_t h2) 
{
    //assert(h2>h1);

    vector<uint64_t> blk_heights(h2-h1);

    std::iota(blk_heights.begin(), blk_heights.end(), h1);

    //for(auto h: blk_heights)
        //cout << h << ",";

    //cout << endl;
    
    COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::response res;

    if (!get_blocks(std::move(blk_heights), res))
    {
        stringstream ss;
       
        ss  << "Getting blocks [" << h1
             << ", " << h2 << ") failed!"; 

        throw std::runtime_error(ss.str());
    }
    
    vector<pair<block, vector<transaction>>> blocks;
    blocks.resize(res.blocks.size());

    //for (auto const& blk_ce: res.blocks)
    //{
    for (uint64_t i = 0; i < res.blocks.size();i++)
    {

      auto const& blk_ce = res.blocks[i];  

      block blk;

      if (!parse_and_validate_block_from_blob(blk_ce.block, blk))
      {
        throw std::runtime_error("Cant parse block from blobdata");
      }

      vector<transaction> txs;
      txs.resize(blk_ce.txs.size());

      //for (auto const& tx_blob: blk_ce.txs)
      for (uint64_t j = 0; j < blk_ce.txs.size(); j++)
      {
          auto const& tx_blob = blk_ce.txs[j];  

          if (!parse_and_validate_tx_from_blob(tx_blob, txs[j]))
          {
            throw std::runtime_error("Cant parse tx from blobdata");
          }
      }

      //blocks.emplace_back(make_pair(std::move(blk), std::move(txs)));
      blocks[i].first = std::move(blk);
      blocks[i].second = std::move(txs);
    }

    return blocks;
}

bool
RPCCalls::get_output_keys(uint64_t amount, 
                vector<uint64_t> const& absolute_offsets,
                vector<output_data_t>& outputs)
{
    COMMAND_RPC_GET_OUTPUTS_BIN::request req;
    COMMAND_RPC_GET_OUTPUTS_BIN::response res;

    req.get_txid = false;

    for (auto ao: absolute_offsets)
    {
        req.outputs.push_back({amount, ao});
    }

    bool r {false};

    {
        std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

        r = epee::net_utils::invoke_http_bin(
                "/get_outs.bin",
                req, res, m_http_client, rpc_timeout);
    }

    string error_msg;

    if (!r || !check_if_response_is_ok(res, error_msg))
    {
        OMERROR << "Error getting output keys. "
                << error_msg;
        return false;
    }

    
    for (auto const& out: res.outs)
    {
        outputs.push_back(output_data_t{});
        
        auto& out_data = outputs.back();
        out_data.pubkey = out.key;
        out_data.height = out.height;
        // out_data.unlock_time = ??? not set
        // out_data.commitment = ?? not set
    }

   
   return true; 

}


bool
RPCCalls::get_tx_amount_output_indices(crypto::hash const& tx_hash,
                            vector<uint64_t>& out_indices)
{
    COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request req;
    COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response res;

    req.txid = tx_hash;

    bool r {false};

    {
        std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

        r = epee::net_utils::invoke_http_bin(
                "/get_o_indexes.bin",
                req, res, m_http_client, rpc_timeout);
    }

    string error_msg;

    if (!r || !check_if_response_is_ok(res, error_msg))
    {
        OMERROR << "Error getting output keys. "
                << error_msg;
        return false;
    }

    out_indices = std::move(res.o_indexes);

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
        COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::response>(
    COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::response const& res,
    string& error_msg) const;

template
bool RPCCalls::check_if_response_is_ok<
        COMMAND_RPC_GET_OUTPUTS_BIN::response>(
    COMMAND_RPC_GET_OUTPUTS_BIN::response const& res,
    string& error_msg) const;

template
bool RPCCalls::check_if_response_is_ok<
        COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response>(
    COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response const& res,
    string& error_msg) const;
}
