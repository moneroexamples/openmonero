//
// Created by mwo on 13/04/16.
//


#ifndef CROWXMR_RPCCALLS_H
#define CROWXMR_RPCCALLS_H

#include "monero_headers.h"

#include <mutex>

namespace xmreg
{

    using namespace cryptonote;
    using namespace crypto;
    using namespace std;


    class rpccalls
    {
        string deamon_url ;
        uint64_t timeout_time;

        std::chrono::milliseconds timeout_time_ms;

        epee::net_utils::http::url_content url;

        epee::net_utils::http::http_simple_client m_http_client;
        std::mutex m_daemon_rpc_mutex;

        string port;

    public:

        rpccalls(string _deamon_url = "http:://127.0.0.1:18081",
                 uint64_t _timeout = 200000)
        : deamon_url {_deamon_url}, timeout_time {_timeout}
        {
            epee::net_utils::parse_url(deamon_url, url);

            port = std::to_string(url.port);

            timeout_time_ms = std::chrono::milliseconds {timeout_time};

            m_http_client.set_server(
                    deamon_url,
                    boost::optional<epee::net_utils::http::login>{});
        }

        bool
        connect_to_monero_deamon()
        {
            if(m_http_client.is_connected())
            {
                return true;
            }

            return m_http_client.connect(timeout_time_ms);
        }

        uint64_t
        get_current_height()
        {
            COMMAND_RPC_GET_HEIGHT::request   req;
            COMMAND_RPC_GET_HEIGHT::response  res;

            std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

            if (!connect_to_monero_deamon())
            {
                cerr << "get_current_height: not connected to deamon" << endl;
                return false;
            }

            bool r = epee::net_utils::invoke_http_json(
                    "/getheight",
                    req, res, m_http_client, timeout_time_ms);

            if (!r)
            {
                cerr << "Error connecting to Monero deamon at "
                     << deamon_url << endl;
                return 0;
            }
            else
            {
                cout << "rpc call /getheight OK: " << endl;
            }

            return res.height;
        }

        bool
        get_mempool(vector<tx_info>& mempool_txs) {

            COMMAND_RPC_GET_TRANSACTION_POOL::request  req;
            COMMAND_RPC_GET_TRANSACTION_POOL::response res;

            std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

            if (!connect_to_monero_deamon())
            {
                cerr << "get_current_height: not connected to deamon" << endl;
                return false;
            }

            bool r = epee::net_utils::invoke_http_json(
                    "/get_transaction_pool",
                    req, res, m_http_client, timeout_time_ms);

            if (!r)
            {
                cerr << "Error connecting to Monero deamon at "
                     << deamon_url << endl;
                return false;
            }


            mempool_txs = res.transactions;

            return true;
        }


        bool
        get_random_outs_for_amounts(const vector<uint64_t>& amounts,
                                    const uint64_t& outs_count,
                                    vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount>& found_outputs,
                                    string& error_msg)
        {
            COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request  req;
            COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response res;

            req.outs_count = outs_count;
            req.amounts = amounts;

            std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

            if (!connect_to_monero_deamon())
            {
                cerr << "get_current_height: not connected to deamon" << endl;
                return false;
            }

            bool r = epee::net_utils::invoke_http_bin(
                    "/getrandom_outs.bin",
                    req, res, m_http_client, timeout_time_ms);


            if (!r || res.status == "Failed")
            {
                cerr << "rpc call to /getrandom_outs.bin failed for some reason" << endl;
                return false;
            }

            if (!res.outs.empty())
            {
                found_outputs = res.outs;
                return true;
            }

            return false;

        }

        /*
         * Not finished. get_random_outs_for_amounts is used instead of this.
         */
        bool
        get_random_outs(const vector<uint64_t>& amounts,
                        const uint64_t& outs_count)
        {


            // get histogram for the amounts we need
            epee::json_rpc::request<COMMAND_RPC_GET_OUTPUT_HISTOGRAM::request>
                    req_t = AUTO_VAL_INIT(req_t);
            epee::json_rpc::response<COMMAND_RPC_GET_OUTPUT_HISTOGRAM::response, std::string>
                    resp_t = AUTO_VAL_INIT(resp_t);

            req_t.jsonrpc = "2.0";
            req_t.id = epee::serialization::storage_entry(0);
            req_t.method = "get_output_histogram";

            req_t.params.amounts = amounts;

            std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

            if (!connect_to_monero_deamon())
            {
                cerr << "get_current_height: not connected to deamon" << endl;
                return false;
            }

            bool r = epee::net_utils::invoke_http_json("/json_rpc",
                                                 req_t, resp_t,
                                                 m_http_client);


            if (!r || resp_t.result.status == "Failed")
            {
                //error_msg = res.reason;

                cerr << "Error get_output_histogram: " << resp_t.result.status << endl;
                return false;
            }


            // generate output indices to request
            COMMAND_RPC_GET_OUTPUTS_BIN::request req = AUTO_VAL_INIT(req);
            COMMAND_RPC_GET_OUTPUTS_BIN::response res = AUTO_VAL_INIT(res);

            for (auto hist: resp_t.result.histogram)
            {
               cout << hist.total_instances << endl;
               req.outputs.push_back({hist.amount, 2});
            }

            r = epee::net_utils::invoke_http_bin("/get_outs.bin",
                                                 req, res, m_http_client,
                                                 timeout_time_ms);

            if (!r || res.status == "Failed")
            {
                //error_msg = res.reason;

                cerr << "Error /get_outs.bin: " << res.status << endl;
                return false;
            }


            for (auto o: res.outs)
            {

                cout << "\no.key: " << pod_to_hex(o.key) << endl;
                cout << "o.mask: " << pod_to_hex(o.mask) << endl;
                cout << "o.txid: " << pod_to_hex(o.txid) << endl;
                cout << "o.height: " << pod_to_hex(o.height) << endl;

                //rct::key mask = td.is_rct() ? rct::commit(td.amount(), td.m_mask) : rct::zeroCommit(td.amount());
                rct::key rct_commitment = rct::zeroCommit(0);
            }

            r;


        }


        bool
        commit_tx(const string& tx_blob, string& error_msg)
        {
            COMMAND_RPC_SEND_RAW_TX::request  req;
            COMMAND_RPC_SEND_RAW_TX::response res;

            req.tx_as_hex = tx_blob;

            req.do_not_relay = false;

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

                cerr << "Error sending tx: " << res.reason << endl;
                return false;
            }

            return true;
        }

        bool
        get_dynamic_per_kb_fee_estimate(uint64_t grace_blocks,
                                        uint64_t& fee,
                                        string& error_msg)
        {
            epee::json_rpc::request<COMMAND_RPC_GET_PER_KB_FEE_ESTIMATE::request>
                    req_t = AUTO_VAL_INIT(req_t);
            epee::json_rpc::response<COMMAND_RPC_GET_PER_KB_FEE_ESTIMATE::response, std::string>
                    resp_t = AUTO_VAL_INIT(resp_t);


            req_t.jsonrpc = "2.0";
            req_t.id = epee::serialization::storage_entry(0);
            req_t.method = "get_fee_estimate";
            req_t.params.grace_blocks = grace_blocks;

            std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

            if (!connect_to_monero_deamon())
            {
                cerr << "get_current_height: not connected to deamon" << endl;
                return false;
            }

            bool r = epee::net_utils::invoke_http_json("/json_rpc",
                                                       req_t, resp_t,
                                                       m_http_client);


            if (!r || resp_t.result.status == "Failed")
            {
                error_msg = resp_t.error;

                cerr << "Error get_dynamic_per_kb_fee_estimate: " << error_msg << endl;

                return false;
            }

            fee = resp_t.result.fee;

            return true;

        }


        bool
        commit_tx(tools::wallet2::pending_tx& ptx, string& error_msg)
        {

            string tx_blob = epee::string_tools::buff_to_hex_nodelimer(
                    tx_to_blob(ptx.tx)
            );

            return commit_tx(tx_blob, error_msg);
        }
    };


}



#endif //CROWXMR_RPCCALLS_H
