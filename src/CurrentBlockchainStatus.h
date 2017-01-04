//
// Created by mwo on 14/12/16.
//

#ifndef RESTBED_XMR_CURRENTBLOCKCHAINSTATUS_H
#define RESTBED_XMR_CURRENTBLOCKCHAINSTATUS_H

#include "mylmdb.h"
#include "rpccalls.h"
#include "tools.h"
//#include "TxSearch.h"

#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>


namespace xmreg {


class TxSearch;


mutex searching_threads_map_mtx;
mutex getting_mempool_txs;

/*
* This is a thread class
*/
struct CurrentBlockchainStatus
{
    static string blockchain_path;

    static atomic<uint64_t> current_height;

    static string deamon_url;

    static bool testnet;

    static std::thread m_thread;

    static bool is_running;

    static uint64_t refresh_block_status_every_seconds;

    // map that will keep track of search threads. In the
    // map, key is address to which a running thread belongs to.
    // make it static to guarantee only one such map exist.
    static map<string, shared_ptr<TxSearch>> searching_threads;

    static string   import_payment_address;
    static string   import_payment_viewkey;
    static uint64_t import_fee;

    static account_public_address address;
    static secret_key             viewkey;

    // vector of mempool transactions that all threads
    // can refer to
    static vector<transaction> mempool_txs;

    // since this class monitors current status
    // of the blockchain, its seems logical to
    // make object for accessing the blockchain here
    static xmreg::MicroCore mcore;
    static cryptonote::Blockchain *core_storage;

    static
    void start_monitor_blockchain_thread()
    {
        bool testnet = CurrentBlockchainStatus::testnet;


        if (!import_payment_address.empty() && !import_payment_viewkey.empty())
        {
            if (!xmreg::parse_str_address(
                    import_payment_address,
                    address,
                    testnet))
            {
                cerr << "Cant parse address_str: "
                     << import_payment_address
                     << endl;
                return;
            }

            if (!xmreg::parse_str_secret_key(
                    import_payment_viewkey,
                    viewkey))
            {
                cerr << "Cant parse the viewkey_str: "
                     << import_payment_viewkey
                     << endl;
                return;
            }
        }

        if (!is_running)
        {
            m_thread = std::thread{[]()
            {
                while (true)
                {
                    current_height = get_current_blockchain_height();
                    read_mempool();
                    cout << "Check block height: " << current_height;
                    cout << " no of mempool txs: " << mempool_txs.size();
                    cout << endl;
                    clean_search_thread_map();
                    std::this_thread::sleep_for(std::chrono::seconds(refresh_block_status_every_seconds));
                }
            }};

            is_running = true;
        }
    }

    static inline
    uint64_t
    get_current_blockchain_height()
    {
        return xmreg::MyLMDB::get_blockchain_height(blockchain_path) - 1;
    }

    static void
    set_blockchain_path(const string &path)
    {
        blockchain_path = path;
    }

    static void
    set_testnet(bool is_testnet)
    {
        testnet = is_testnet;
    }

    static bool
    init_monero_blockchain()
    {
        // enable basic monero log output
        xmreg::enable_monero_log();

        // initialize mcore and core_storage
        if (!xmreg::init_blockchain(blockchain_path,
                                    mcore, core_storage))
        {
            cerr << "Error accessing blockchain." << endl;
            return false;
        }

        return true;
    }

    static bool
    get_block(uint64_t height, block &blk)
    {
        return mcore.get_block_from_height(height, blk);
    }

    static bool
    get_block_txs(const block &blk, list <transaction> &blk_txs)
    {
        // get all transactions in the block found
        // initialize the first list with transaction for solving
        // the block i.e. coinbase tx.
        blk_txs.push_back(blk.miner_tx);

        list <crypto::hash> missed_txs;

        if (!core_storage->get_transactions(blk.tx_hashes, blk_txs, missed_txs)) {
            cerr << "Cant get transactions in block: " << get_block_hash(blk) << endl;
            return false;
        }

        return true;
    }

    static bool
    get_output_keys(const uint64_t& amount,
                    const vector<uint64_t>& absolute_offsets,
                    vector<cryptonote::output_data_t>& outputs)
    {
        try
        {
            core_storage->get_db().get_output_key(amount,
                                                  absolute_offsets,
                                                  outputs);
        }
        catch (const OUTPUT_DNE& e)
        {
            cerr << "get_output_keys: " << e.what() << endl;
            return false;
        }

        return true;
    }

    static bool
    get_amount_specific_indices(const crypto::hash& tx_hash,
                                vector<uint64_t>& out_indices)
    {
        try
        {
            // this index is lmdb index of a tx, not tx hash
            uint64_t tx_index;

            if (core_storage->get_db().tx_exists(tx_hash, tx_index))
            {
                out_indices = core_storage->get_db()
                        .get_tx_amount_output_indices(tx_index);

                return true;
            }
        }
        catch(const exception& e)
        {
            cerr << e.what() << endl;
        }

        return false;
    }

    static bool
    get_random_outputs(const vector<uint64_t>& amounts,
                       const uint64_t& outs_count,
                       vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount>& found_outputs)
    {
        rpccalls rpc {deamon_url};

        string error_msg;

        if (!rpc.get_random_outs_for_amounts(amounts, outs_count, found_outputs, error_msg))
        {
            cerr << "rpc.get_random_outs_for_amounts failed" << endl;
            return false;
        }

        return true;
    }

    static bool
    commit_tx(const string& tx_blob)
    {
        rpccalls rpc {deamon_url};

        string error_msg;

        if (!rpc.commit_tx(tx_blob, error_msg))
        {
            cerr << "commit_tx failed" << endl;
            return false;
        }

        return true;
    }

    static bool
    read_mempool()
    {
        rpccalls rpc {deamon_url};

        string error_msg;

        std::lock_guard<std::mutex> lck (getting_mempool_txs);

        // clear current mempool txs vector
        // repopulate it with each execution of read_mempool()
        // not very efficient but good enough for now.
        mempool_txs.clear();

        // get txs in the mempool
        std::vector<tx_info> mempool_tx_info;

        if (!rpc.get_mempool(mempool_tx_info))
        {
            cerr << "Getting mempool failed " << endl;
            return false;
        }

        // if dont have tx_blob member, construct tx
        // from json obtained from the rpc call

        for (size_t i = 0; i < mempool_tx_info.size(); ++i)
        {
            // get transaction info of the tx in the mempool
            tx_info _tx_info = mempool_tx_info.at(i);

            crypto::hash mem_tx_hash = null_hash;

            if (hex_to_pod(_tx_info.id_hash, mem_tx_hash))
            {
                transaction tx;

                if (!xmreg::make_tx_from_json(_tx_info.tx_json, tx))
                {
                    cerr << "Cant make tx from _tx_info.tx_json" << endl;
                    return false;
                }

                if (_tx_info.id_hash != pod_to_hex(get_transaction_hash(tx)))
                {
                    cerr << "Hash of reconstructed tx from json does not match "
                            "what we should get!"
                         << endl;

                    return false;
                }

                mempool_txs.push_back(tx);

            } // if (hex_to_pod(_tx_info.id_hash, mem_tx_hash))

        } // for (size_t i = 0; i < mempool_tx_info.size(); ++i)

        return true;
    }

    static vector<transaction>
    get_mempool_txs()
    {
        std::lock_guard<std::mutex> lck (getting_mempool_txs);
        return mempool_txs;
    }

    static bool
    search_if_payment_made(
            const string& payment_id_str,
            const uint64_t& desired_amount,
            string& tx_hash_with_payment)
    {

        crypto::hash payment_id;

        if (!hex_to_pod(payment_id_str, payment_id))
        {
            cerr << "Cant parse the payment_id_str: " << payment_id_str << endl;
            return false;
        }

        vector<transaction> txs_to_check = get_mempool_txs();

        uint64_t current_blockchain_height = current_height;

        // apend txs in last to blocks into the txs_to_check vector
        for (uint64_t blk_i = current_blockchain_height - 2;
             blk_i <= current_blockchain_height;
             ++blk_i)
        {
            // get block cointaining this tx
            block blk;

            if (!get_block(blk_i, blk)) {
                cerr << "Cant get block of height: " + to_string(blk_i) << endl;
                return false;
            }

            list <cryptonote::transaction> blk_txs;

            if (!get_block_txs(blk, blk_txs))
            {
                cerr << "Cant get transactions in block: " << to_string(blk_i) << endl;
                return false;
            }

            txs_to_check.insert(txs_to_check.end(), blk_txs.begin(), blk_txs.end());
        }

        for (transaction& tx: txs_to_check)
        {
            if (is_coinbase(tx))
            {
                // not interested in coinbase txs
                continue;
            }

            public_key tx_pub_key = xmreg::get_tx_pub_key_from_received_outs(tx);

            //          <public_key  , amount  , out idx>
            vector<tuple<txout_to_key, uint64_t, uint64_t>> outputs;

            outputs = get_ouputs_tuple(tx);

            // for each output, in a tx, check if it belongs
            // to the given account of specific address and viewkey

            // public transaction key is combined with our viewkey
            // to create, so called, derived key.
            key_derivation derivation;

            if (!generate_key_derivation(tx_pub_key, viewkey, derivation))
            {
                cerr << "Cant get derived key for: "  << "\n"
                     << "pub_tx_key: " << tx_pub_key << " and "
                     << "prv_view_key" << viewkey << endl;

                return false;
            }

            string tx_hash_str = pod_to_hex(get_transaction_hash(tx));



            uint64_t total_received {0};

            for (auto& out: outputs)
            {
                txout_to_key txout_k = std::get<0>(out);
                uint64_t amount = std::get<1>(out);
                uint64_t output_idx_in_tx = std::get<2>(out);

                // get the tx output public key
                // that normally would be generated for us,
                // if someone had sent us some xmr.
                public_key generated_tx_pubkey;

                derive_public_key(derivation,
                                  output_idx_in_tx,
                                  address.m_spend_public_key,
                                  generated_tx_pubkey);

                // check if generated public key matches the current output's key
                bool mine_output = (txout_k.key == generated_tx_pubkey);

                if (mine_output)
                {
                    total_received += amount;
                }
            }

            cout << " - payment id check in tx: "
                 << tx_hash_str
                 << " found: " << total_received << endl;

            if (total_received == desired_amount)
            {
                // the payment has been made.
                tx_hash_with_payment = tx_hash_str;
                return true;
            }
        }

        return false;
    }

        // definitions of these function are at the end of this file
    // due to forward declaraions of TxSearch
    static bool
    start_tx_search_thread(XmrAccount acc);

    static bool
    ping_search_thread(const string& address);

    static void
    clean_search_thread_map();

};


class TxSearchException: public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

class TxSearch
{

    // how frequently update scanned_block_height in Accounts table
    static constexpr uint64_t UPDATE_SCANNED_HEIGHT_INTERVAL = 10; // seconds

    // how long should the search thread be live after no request
    // are coming from the frontend. For example, when a user finishes
    // using the service.
    static constexpr uint64_t THREAD_LIFE_DURATION           = 10 * 60; // in seconds


    bool continue_search {true};


    uint64_t last_ping_timestamp;


    // represents a row in mysql's Accounts table
    XmrAccount acc;

    // this manages all mysql queries
    // its better to when each thread has its own mysql connection object.
    // this way if one thread crashes, it want take down
    // connection for the entire service
    shared_ptr<MySqlAccounts> xmr_accounts;

    // address and viewkey for this search thread.
    account_public_address address;
    secret_key viewkey;

public:

    TxSearch(XmrAccount& _acc):
            acc {_acc}
    {

        xmr_accounts = make_shared<MySqlAccounts>();

        bool testnet = CurrentBlockchainStatus::testnet;

        if (!xmreg::parse_str_address(acc.address, address, testnet))
        {
            cerr << "Cant parse string address: " << acc.address << endl;
            throw TxSearchException("Cant parse string address: " + acc.address);
        }

        if (!xmreg::parse_str_secret_key(acc.viewkey, viewkey))
        {
            cerr << "Cant parse the private key: " << acc.viewkey << endl;
            throw TxSearchException("Cant parse private key: " + acc.viewkey);
        }

        ping();
    }

    void
    search()
    {

        // start searching from last block that we searched for
        // this accont
        uint64_t searched_blk_no = acc.scanned_block_height;

        if (searched_blk_no > CurrentBlockchainStatus::current_height)
        {
            throw TxSearchException("searched_blk_no > CurrentBlockchainStatus::current_height");
        }

        uint64_t current_timestamp = chrono::duration_cast<chrono::seconds>(
                chrono::system_clock::now().time_since_epoch()).count();


        uint64_t loop_idx {0};

        while(continue_search)
        {
            ++loop_idx;

            uint64_t loop_timestamp {current_timestamp};

            if (loop_idx % 2 == 0)
            {
                // get loop time every second iteration. no need to call it
                // all the time.
                loop_timestamp = chrono::duration_cast<chrono::seconds>(
                        chrono::system_clock::now().time_since_epoch()).count();

                //cout << "loop_timestamp: " <<  loop_timestamp << endl;
                //cout << "last_ping_timestamp: " <<  last_ping_timestamp << endl;
                //cout << "loop_timestamp - last_ping_timestamp: " <<  (loop_timestamp - last_ping_timestamp) << endl;

                if (loop_timestamp - last_ping_timestamp > THREAD_LIFE_DURATION)
                {
                    // also check if we caught up with current blockchain height
                    if (searched_blk_no == CurrentBlockchainStatus::current_height)
                    {
                        stop();
                        continue;
                    }
                }
            }



            if (searched_blk_no > CurrentBlockchainStatus::current_height) {
                fmt::print("searched_blk_no {:d} and current_height {:d}\n",
                           searched_blk_no, CurrentBlockchainStatus::current_height);

                std::this_thread::sleep_for(
                        std::chrono::seconds(
                                CurrentBlockchainStatus::refresh_block_status_every_seconds)
                );

                continue;
            }

            //
            //cout << " - searching tx of: " << acc << endl;

            // get block cointaining this tx
            block blk;

            if (!CurrentBlockchainStatus::get_block(searched_blk_no, blk)) {
                cerr << "Cant get block of height: " + to_string(searched_blk_no) << endl;
                //searched_blk_no = -2; // just go back one block, and retry
                continue;
            }

            // for each tx in the given block look, get ouputs


            list <cryptonote::transaction> blk_txs;

            if (!CurrentBlockchainStatus::get_block_txs(blk, blk_txs)) {
                throw TxSearchException("Cant get transactions in block: " + to_string(searched_blk_no));
            }


            if (searched_blk_no % 100 == 0)
            {
                // print status every 10th block

                fmt::print(" - searching block  {:d} of hash {:s} \n",
                           searched_blk_no, pod_to_hex(get_block_hash(blk)));
            }



            // searching for our incoming and outgoing xmr has two componotes.
            //
            // FIRST. to search for the incoming xmr, we use address, viewkey and
            // outputs public key. Its stright forward.
            // second. searching four our spendings is trickier, as we dont have
            //
            // SECOND. But what we can do, we can look for condidate spend keys.
            // and this can be achieved by checking if any mixin in associated with
            // the given image, is our output. If it is our output, than we assume
            // its our key image (i.e. we spend this output). Off course this is only
            // assumption as our outputs can be used in key images of others for their
            // mixin purposes. Thus, we sent to the front end, the list of key images
            // that we think are yours, and the frontend, because it has spend key,
            // can filter out false positives.
            for (transaction& tx: blk_txs)
            {

                crypto::hash tx_hash         = get_transaction_hash(tx);
                crypto::hash tx_prefix_hash  = get_transaction_prefix_hash(tx);

                string tx_hash_str           = pod_to_hex(tx_hash);
                string tx_prefix_hash_str    = pod_to_hex(tx_prefix_hash);

                bool is_coinbase_tx = is_coinbase(tx);

                vector<uint64_t> amount_specific_indices;

                // cout << pod_to_hex(tx_hash) << endl;

                public_key tx_pub_key = xmreg::get_tx_pub_key_from_received_outs(tx);

                //          <public_key  , amount  , out idx>
                vector<tuple<txout_to_key, uint64_t, uint64_t>> outputs;

                outputs = get_ouputs_tuple(tx);

                // for each output, in a tx, check if it belongs
                // to the given account of specific address and viewkey

                // public transaction key is combined with our viewkey
                // to create, so called, derived key.
                key_derivation derivation;

                if (!generate_key_derivation(tx_pub_key, viewkey, derivation))
                {
                    cerr << "Cant get derived key for: "  << "\n"
                         << "pub_tx_key: " << tx_pub_key << " and "
                         << "prv_view_key" << viewkey << endl;

                    throw TxSearchException("");
                }

                uint64_t total_received {0};


                // FIRST component: Checking for our outputs.

                //     <out_pub_key, amount  , index in tx>
                vector<tuple<string, uint64_t, uint64_t>> found_mine_outputs;

                for (auto& out: outputs)
                {
                    txout_to_key txout_k      = std::get<0>(out);
                    uint64_t amount           = std::get<1>(out);
                    uint64_t output_idx_in_tx = std::get<2>(out);

                    // get the tx output public key
                    // that normally would be generated for us,
                    // if someone had sent us some xmr.
                    public_key generated_tx_pubkey;

                    derive_public_key(derivation,
                                      output_idx_in_tx,
                                      address.m_spend_public_key,
                                      generated_tx_pubkey);

                    // check if generated public key matches the current output's key
                    bool mine_output = (txout_k.key == generated_tx_pubkey);


                    //cout  << "Chekcing output: "  << pod_to_hex(txout_k.key) << " "
                    //      << "mine_output: " << mine_output << endl;


                    // if mine output has RingCT, i.e., tx version is 2
                    // need to decode its amount. otherwise its zero.
                    if (mine_output && tx.version == 2)
                    {
                        // initialize with regular amount
                        uint64_t rct_amount = amount;



                        // cointbase txs have amounts in plain sight.
                        // so use amount from ringct, only for non-coinbase txs
                        if (!is_coinbase_tx)
                        {
                            bool r;

                            r = decode_ringct(tx.rct_signatures,
                                              tx_pub_key,
                                              viewkey,
                                              output_idx_in_tx,
                                              tx.rct_signatures.ecdhInfo[output_idx_in_tx].mask,
                                              rct_amount);

                            if (!r)
                            {
                                cerr << "Cant decode ringCT!" << endl;
                                throw TxSearchException("Cant decode ringCT!");
                            }

                            rct_amount = amount;
                        }

                        amount = rct_amount;

                    } // if (mine_output && tx.version == 2)

                    if (mine_output)
                    {

                        string out_key_str = pod_to_hex(txout_k.key);

                        // found an output associated with the given address and viewkey
                        string msg = fmt::format("block: {:d}, tx_hash:  {:s}, output_pub_key: {:s}\n",
                                                 searched_blk_no,
                                                 pod_to_hex(tx_hash),
                                                 out_key_str);

                        cout << msg << endl;


                        total_received += amount;

                        found_mine_outputs.emplace_back(out_key_str,
                                                        amount,
                                                        output_idx_in_tx);
                    }

                } // for (const auto& out: outputs)

                DateTime blk_timestamp_mysql_format
                        = XmrTransaction::timestamp_to_DateTime(blk.timestamp);

                uint64_t tx_mysql_id {0};

                if (!found_mine_outputs.empty())
                {

                    XmrTransaction tx_data;

                    tx_data.hash           = tx_hash_str;
                    tx_data.prefix_hash    = tx_prefix_hash_str;
                    tx_data.account_id     = acc.id;
                    tx_data.total_received = total_received;
                    tx_data.total_sent     = 0; // at this stage we don't have any
                    // info about spendings
                    tx_data.unlock_time    = 0;
                    tx_data.height         = searched_blk_no;
                    tx_data.coinbase       = is_coinbase_tx;
                    tx_data.payment_id     = get_payment_id_as_string(tx);
                    tx_data.mixin          = get_mixin_no(tx) - 1;
                    tx_data.timestamp      = blk_timestamp_mysql_format;

                    // insert tx_data into mysql's Transactions table
                    tx_mysql_id = xmr_accounts->insert_tx(tx_data);

                    // get amount specific (i.e., global) indices of outputs

                    if (!CurrentBlockchainStatus::get_amount_specific_indices(tx_hash,
                                                                              amount_specific_indices))
                    {
                        cerr << "cant get_amount_specific_indices!" << endl;
                        throw TxSearchException("cant get_amount_specific_indices!");
                    }

                    if (tx_mysql_id == 0)
                    {
                        //cerr << "tx_mysql_id is zero!" << endl;
                        //throw TxSearchException("tx_mysql_id is zero!");
                    }

                    // now add the found outputs into Outputs tables

                    for (auto &out_k_idx: found_mine_outputs)
                    {
                        XmrOutput out_data;

                        out_data.account_id   = acc.id;
                        out_data.tx_id        = tx_mysql_id;
                        out_data.out_pub_key  = std::get<0>(out_k_idx);
                        out_data.tx_pub_key   = pod_to_hex(tx_pub_key);
                        out_data.amount       = std::get<1>(out_k_idx);
                        out_data.out_index    = std::get<2>(out_k_idx);
                        out_data.global_index = amount_specific_indices.at(out_data.out_index);
                        out_data.mixin        = tx_data.mixin;
                        out_data.timestamp    = tx_data.timestamp;

                        // insert output into mysql's outputs table
                        uint64_t out_mysql_id = xmr_accounts->insert_output(out_data);

                        if (out_mysql_id == 0)
                        {
                            //cerr << "out_mysql_id is zero!" << endl;
                            //throw TxSearchException("out_mysql_id is zero!");
                        }
                    } // for (auto &out_k_idx: found_mine_outputs)



                    // once tx and outputs were added, update Accounts table

                    XmrAccount updated_acc = acc;

                    updated_acc.total_received = acc.total_received + tx_data.total_received;

                    if (xmr_accounts->update(acc, updated_acc))
                    {
                        // if success, set acc to updated_acc;
                        acc = updated_acc;
                    }

                } // if (!found_mine_outputs.empty())



                // SECOND component: Checking for our key images, i.e., inputs.

                vector<txin_to_key> input_key_imgs = xmreg::get_key_images(tx);

                // here we will keep what we find.
                vector<XmrInput> inputs_found;

                // make timescale maps for mixins in input
                for (const txin_to_key& in_key: input_key_imgs)
                {
                    // get absolute offsets of mixins
                    std::vector<uint64_t> absolute_offsets
                            = cryptonote::relative_output_offsets_to_absolute(
                                    in_key.key_offsets);

                    // get public keys of outputs used in the mixins that match to the offests
                    std::vector<cryptonote::output_data_t> mixin_outputs;


                    if (!CurrentBlockchainStatus::get_output_keys(in_key.amount,
                                                                  absolute_offsets,
                                                                  mixin_outputs))
                    {
                        cerr << "Mixins key images not found" << endl;
                        continue;
                    }


                    // for each found output public key find check if its ours or not
                    for (const cryptonote::output_data_t& output_data: mixin_outputs)
                    {
                        string output_public_key_str = pod_to_hex(output_data.pubkey);

                        XmrOutput out;

                        if (xmr_accounts->output_exists(output_public_key_str, out))
                        {
                            cout << "input uses some mixins which are our outputs"
                                 << out << endl;

                            // seems that this key image is ours.
                            // so save it to database for later use.



                            XmrInput in_data;

                            in_data.account_id  = acc.id;
                            in_data.tx_id       = 0; // for now zero, later we set it
                            in_data.output_id   = out.id;
                            in_data.key_image   = pod_to_hex(in_key.k_image);
                            in_data.amount      = in_key.amount;
                            in_data.timestamp   = blk_timestamp_mysql_format;

                            inputs_found.push_back(in_data);

                            // a key image has only one real mixin. Rest is fake.
                            // so if we find a candidate, break the search.

                            // break;

                        } // if (xmr_accounts->output_exists(output_public_key_str, out))

                    } // for (const cryptonote::output_data_t& output_data: outputs)

                } // for (const txin_to_key& in_key: input_key_imgs)


                if (!inputs_found.empty())
                {
                    // seems we have some inputs found. time
                    // to write it to mysql. But first,
                    // check if this tx is written in mysql.


                    // calculate how much we preasumply spent.
                    uint64_t total_sent {0};

                    for (const XmrInput& in_data: inputs_found)
                    {
                        total_sent += in_data.amount;
                    }


                    if (tx_mysql_id == 0)
                    {
                        // this txs hasnt been seen in step first.
                        // it means that it only contains potentially our
                        // key images. It does not have our outputs.
                        // so write it to mysql as ours, with
                        // total received of 0.

                        XmrTransaction tx_data;

                        tx_data.hash           = tx_hash_str;
                        tx_data.prefix_hash    = tx_prefix_hash_str;
                        tx_data.account_id     = acc.id;
                        tx_data.total_received = 0;
                        tx_data.total_sent     = total_sent;
                        tx_data.unlock_time    = 0;
                        tx_data.height         = searched_blk_no;
                        tx_data.coinbase       = is_coinbase(tx);
                        tx_data.payment_id     = get_payment_id_as_string(tx);
                        tx_data.mixin          = get_mixin_no(tx) - 1;
                        tx_data.timestamp      = blk_timestamp_mysql_format;

                        // insert tx_data into mysql's Transactions table
                        tx_mysql_id = xmr_accounts->insert_tx(tx_data);

                        if (tx_mysql_id == 0)
                        {
                            //cerr << "tx_mysql_id is zero!" << endl;
                            //throw TxSearchException("tx_mysql_id is zero!");
                        }
                    }

                } //  if (!inputs_found.empty())


                // save all input found into database
                for (XmrInput& in_data: inputs_found)
                {
                    in_data.tx_id = tx_mysql_id;
                    uint64_t in_mysql_id = xmr_accounts->insert_input(in_data);
                }


            } // for (const transaction& tx: blk_txs)


            if ((loop_timestamp - current_timestamp > UPDATE_SCANNED_HEIGHT_INTERVAL)
                || searched_blk_no == CurrentBlockchainStatus::current_height)
            {
                // update scanned_block_height every given interval
                // or when we reached top of the blockchain

                XmrAccount updated_acc = acc;

                updated_acc.scanned_block_height = searched_blk_no;

                if (xmr_accounts->update(acc, updated_acc))
                {
                    // iff success, set acc to updated_acc;
                    cout << "scanned_block_height updated"  << endl;
                    acc = updated_acc;
                }

                current_timestamp = loop_timestamp;
            }

            ++searched_blk_no;

        } // while(continue_search)
    }

    void
    stop()
    {
        cout << "Stoping the thread by setting continue_search=false" << endl;
        continue_search = false;
    }

    ~TxSearch()
    {
        cout << "TxSearch destroyed" << endl;
    }


    string
    get_payment_id_as_string(const transaction& tx)
    {
        crypto::hash payment_id = null_hash;
        crypto::hash8 payment_id8 = null_hash8;

        get_payment_id(tx, payment_id, payment_id8);

        string payment_id_str{""};

        if (payment_id != null_hash)
        {
            payment_id_str = pod_to_hex(payment_id);
        }
        else if (payment_id8 != null_hash8)
        {
            payment_id_str = pod_to_hex(payment_id8);
        }

        return payment_id_str;
    }

    void
    ping()
    {
        cout << "new last_ping_timestamp: " << last_ping_timestamp << endl;

        last_ping_timestamp = chrono::duration_cast<chrono::seconds>(
                chrono::system_clock::now().time_since_epoch()).count();
    }

    bool
    still_searching()
    {
        return continue_search;
    }


};


// initialize static variables
atomic<uint64_t>        CurrentBlockchainStatus::current_height{0};
string                  CurrentBlockchainStatus::blockchain_path{"/home/mwo/.blockchain/lmdb"};
string                  CurrentBlockchainStatus::deamon_url{"http:://127.0.0.1:18081"};
bool                    CurrentBlockchainStatus::testnet{false};
bool                    CurrentBlockchainStatus::is_running{false};
std::thread             CurrentBlockchainStatus::m_thread;
uint64_t                CurrentBlockchainStatus::refresh_block_status_every_seconds{60};
xmreg::MicroCore        CurrentBlockchainStatus::mcore;
cryptonote::Blockchain *CurrentBlockchainStatus::core_storage;
vector<transaction>     CurrentBlockchainStatus::mempool_txs;
string                  CurrentBlockchainStatus::import_payment_address;
string                  CurrentBlockchainStatus::import_payment_viewkey;
uint64_t                CurrentBlockchainStatus::import_fee {10000000000}; // 0.01 xmr
account_public_address  CurrentBlockchainStatus::address;
secret_key              CurrentBlockchainStatus::viewkey;
map<string, shared_ptr<TxSearch>> CurrentBlockchainStatus::searching_threads;


bool
CurrentBlockchainStatus::start_tx_search_thread(XmrAccount acc)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (searching_threads.count(acc.address) > 0)
    {
        // thread for this address exist, dont make new one
        cout << "Thread exisist, dont make new one" << endl;
        return false;
    }

    // make a tx_search object for the given xmr account
    searching_threads[acc.address] = make_shared<TxSearch>(acc);

    // start the thread for the created object
    std::thread t1 {&TxSearch::search, searching_threads[acc.address].get()};
    t1.detach();

    return true;
}

bool
CurrentBlockchainStatus::ping_search_thread(const string& address)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (searching_threads.count(address) == 0)
    {
        // thread does not exist
        cout << "does not exist" << endl;
        return false;
    }

    searching_threads[address].get()->ping();

    return true;
}


void
CurrentBlockchainStatus::clean_search_thread_map()
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    for (auto st: searching_threads)
    {
        if (st.second->still_searching() == false)
        {
            cout << st.first << " still searching: " << st.second->still_searching() << endl;
            searching_threads.erase(st.first);
        }
    }
}


}
#endif //RESTBED_XMR_CURRENTBLOCKCHAINSTATUS_H
