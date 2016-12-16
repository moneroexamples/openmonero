//
// Created by mwo on 12/12/16.
//

#ifndef RESTBED_XMR_TXSEARCH_H
#define RESTBED_XMR_TXSEARCH_H



#include "MySqlAccounts.h"
#include "tools.h"
#include "mylmdb.h"
#include "CurrentBlockchainStatus.h"

#include <mysql++/mysql++.h>
#include <mysql++/ssqls.h>


#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

namespace xmreg
{

mutex mtx;


class TxSearchException: public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

class TxSearch
{

    static constexpr uint64_t UPDATE_SCANNED_HEIGHT_INTERVAL = 10; // seconds


    bool continue_search {true};

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

    }

    void
    search()
    {

        // start searching from last block that we searched for
        // this accont
        uint64_t searched_blk_no = acc.scanned_block_height;

        // start scanning from befor for tests.
        searched_blk_no -= 100;

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
                searched_blk_no = -2; // just go back one block, and retry
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
            // FIRIST. to search for the incoming xmr, we use address, viewkey and
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

                crypto::hash tx_hash  = get_transaction_hash(tx);

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

                //     <out_pub_key, index in tx>
                vector<pair<string, uint64_t>> found_mine_outputs;

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
                            throw TxSearchException("");
                        }

                        // cointbase txs have amounts in plain sight.
                        // so use amount from ringct, only for non-coinbase txs
                        if (!is_coinbase(tx))
                        {
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

                        found_mine_outputs.emplace_back(out_key_str, output_idx_in_tx);
                    }

                } // for (const auto& out: outputs)

                if (!found_mine_outputs.empty()) {

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

                    string tx_hash_str = pod_to_hex(tx_hash);

                    XmrTransaction tx_data;

                    tx_data.hash = tx_hash_str;
                    tx_data.account_id = acc.id;
                    tx_data.total_received = total_received;
                    tx_data.total_sent = 0; // at this stage we don't have any
                                            // info about spendings
                    tx_data.unlock_time = 0;
                    tx_data.height = searched_blk_no;
                    tx_data.coinbase = is_coinbase(tx);
                    tx_data.payment_id = payment_id_str;
                    tx_data.mixin = get_mixin_no(tx) - 1;
                    tx_data.timestamp = XmrTransaction::timestamp_to_DateTime(blk.timestamp);

                    // insert tx_data into mysql's Transactions table
                    uint64_t tx_mysql_id = xmr_accounts->insert_tx(tx_data);

                    if (tx_mysql_id == 0)
                    {
                        cerr << "tx_mysql_id is zero!" << endl;
                        throw TxSearchException("tx_mysql_id is zero!");
                    }

                    // now add the found outputs into Outputs tables

                    for (auto &out_k_idx: found_mine_outputs)
                    {
                        XmrOutput out_data;

                        out_data.account_id   = acc.id;
                        out_data.tx_id        = tx_mysql_id;
                        out_data.out_pub_key  = out_k_idx.first;
                        out_data.tx_pub_key   = pod_to_hex(tx_pub_key);
                        out_data.out_index    = out_k_idx.second;
                        out_data.mixin        = tx_data.mixin;
                        out_data.timestamp    = tx_data.timestamp;

                        // insert output into mysql's outputs table
                        uint64_t out_mysql_id = xmr_accounts->insert_output(out_data);

                        if (out_mysql_id == 0)
                        {
                            cerr << "out_mysql_id is zero!" << endl;
                            throw TxSearchException("out_mysql_id is zero!");
                        }
                    }


                    // SECOND component: Checking for our key images, i.e., inputs.





                    // once tx and outputs were added, update Accounts table

                    XmrAccount updated_acc = acc;

                    updated_acc.total_received = acc.total_received + tx_data.total_received;

                    if (xmr_accounts->update(acc, updated_acc))
                    {
                        // iff success, set acc to updated_acc;
                        acc = updated_acc;
                    }

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

            //std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    void
    stop()
    {
        cout << "something to stop the thread by setting continue_search=false" << endl;
    }

    ~TxSearch()
    {
        cout << "TxSearch destroyed" << endl;
    }




};

}

#endif //RESTBED_XMR_TXSEARCH_H
