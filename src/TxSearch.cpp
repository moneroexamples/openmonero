//
// Created by mwo on 8/01/17.
//

#define MYSQLPP_SSQLS_NO_STATICS 1

#include "TxSearch.h"

#include "MySqlAccounts.h"

#include "ssqlses.h"

#include "CurrentBlockchainStatus.h"

namespace xmreg
{



TxSearch::TxSearch(XmrAccount& _acc)
{
    acc = make_shared<XmrAccount>(_acc);

    xmr_accounts = make_shared<MySqlAccounts>();

    bool testnet = CurrentBlockchainStatus::testnet;

    if (!xmreg::parse_str_address(acc->address, address, testnet))
    {
        cerr << "Cant parse string address: " << acc->address << endl;
        throw TxSearchException("Cant parse string address: " + acc->address);
    }

    if (!xmreg::parse_str_secret_key(acc->viewkey, viewkey))
    {
        cerr << "Cant parse the private key: " << acc->viewkey << endl;
        throw TxSearchException("Cant parse private key: " + acc->viewkey);
    }

    populate_known_outputs();

    // start searching from last block that we searched for
    // this accont
    searched_blk_no = acc->scanned_block_height;

    ping();
}

void
TxSearch::search()
{

    if (searched_blk_no > CurrentBlockchainStatus::current_height)
    {
        throw TxSearchException("searched_blk_no > CurrentBlockchainStatus::current_height");
    }

    uint64_t current_timestamp = chrono::duration_cast<chrono::seconds>(
            chrono::system_clock::now().time_since_epoch()).count();

    last_ping_timestamp = current_timestamp;


    uint64_t loop_idx {0};

    while(continue_search)
    {
        ++loop_idx;

        uint64_t loop_timestamp {current_timestamp};

        if (loop_idx % 5 == 0)
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
            --searched_blk_no; // just go back one block, and retry
                               // maybe this was laternative block and got orphaned.
            continue;
        }

        // get all txs in the block
        list <cryptonote::transaction> blk_txs;

        if (!CurrentBlockchainStatus::get_block_txs(blk, blk_txs))
        {
            throw TxSearchException("Cant get transactions in block: " + to_string(searched_blk_no));
        }


        if (searched_blk_no % 100 == 0)
        {
            // print status every 10th block

            fmt::print(" - searching block  {:d} of hash {:s} \n",
                       searched_blk_no, pod_to_hex(get_block_hash(blk)));
        }

        DateTime blk_timestamp_mysql_format
                = XmrTransaction::timestamp_to_DateTime(blk.timestamp);

        // searching for our incoming and outgoing xmr has two components.
        //
        // FIRST. to search for the incoming xmr, we use address, viewkey and
        // outputs public key. Its straight forward, as this is what viewkey was
        // designed to do.
        //
        // SECOND. Searching for spendings (i.e., key images) is more difficult,
        // because we dont have spendkey. But what we can do is, we can look for
        // candidate key images. And this can be achieved by checking if any mixin
        // in associated with the given key image, is our output. If it is our output,
        // then we assume its our key image (i.e. we spend this output). Off course this is only
        // assumption as our outputs can be used in key images of others for their
        // mixin purposes. Thus, we sent to the front end the list of key images
        // that we think are yours, and the frontend, because it has spend key,
        // can filter out false positives.
        for (transaction& tx: blk_txs)
        {
            // Class that is resposnible for idenficitaction of our outputs
            // and inputs in a given tx.
            OutputInputIdentification oi_identification {&address, &viewkey, &tx};

            // flag indicating whether the txs in the given block are spendable.
            // this is true when block number is more than 10 blocks from current
            // blockchain height.

            bool is_spendable = CurrentBlockchainStatus::is_tx_unlocked(
                    tx.unlock_time, searched_blk_no);

            // FIRSt step.
            oi_identification.identify_outputs();

            vector<uint64_t> amount_specific_indices;

            uint64_t tx_mysql_id {0};

            // if we identified some outputs as ours,
            // save them into mysql.
            if (!oi_identification.identified_outputs.empty())
            {
                // before adding this tx and its outputs to mysql
                // check if it already exists. So that we dont
                // do it twice.

                XmrTransaction tx_data;

                if (xmr_accounts->tx_exists(acc->id,
                                            oi_identification.tx_hash_str,
                                            tx_data))
                {
                    cout << "\nTransaction " << oi_identification.tx_hash_str
                         << " already present in mysql"
                         << endl;
                }

                tx_data.hash           = oi_identification.tx_hash_str;
                tx_data.prefix_hash    = oi_identification.tx_prefix_hash_str;
                tx_data.account_id     = acc->id;
                tx_data.total_received = oi_identification.total_received;
                tx_data.total_sent     = 0; // at this stage we don't have any
                                            // info about spendings

                                         // this is current block + unlock time
                                         // for regular tx, the unlock time is
                                         // default of 10 blocks.
                                         // for coinbase tx it is 60 blocks
                tx_data.unlock_time    = tx.unlock_time;

                tx_data.height         = searched_blk_no;
                tx_data.coinbase       = oi_identification.tx_is_coinbase;
                tx_data.is_rct         = oi_identification.is_rct;
                tx_data.rct_type       = oi_identification.rct_type;
                tx_data.spendable      = is_spendable;
                tx_data.payment_id     = CurrentBlockchainStatus::get_payment_id_as_string(tx);
                tx_data.mixin          = oi_identification.mixin_no;
                tx_data.timestamp      = blk_timestamp_mysql_format;


                // insert tx_data into mysql's Transactions table
                tx_mysql_id = xmr_accounts->insert_tx(tx_data);

                // get amount specific (i.e., global) indices of outputs
                if (!CurrentBlockchainStatus::get_amount_specific_indices(
                        oi_identification.tx_hash, amount_specific_indices))
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
                for (auto& out_info: oi_identification.identified_outputs)
                {
                    XmrOutput out_data;

                    out_data.account_id   = acc->id;
                    out_data.tx_id        = tx_mysql_id;
                    out_data.out_pub_key  = out_info.pub_key;
                    out_data.tx_pub_key   = oi_identification.tx_pub_key_str;
                    out_data.amount       = out_info.amount;
                    out_data.out_index    = out_info.idx_in_tx;
                    out_data.rct_outpk    = out_info.rtc_outpk;
                    out_data.rct_mask     = out_info.rtc_mask;
                    out_data.rct_amount   = out_info.rtc_amount;
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

                    {
                        std::lock_guard<std::mutex> lck (getting_known_outputs_keys);
                        known_outputs_keys.push_back(make_pair(out_info.pub_key, out_info.amount));
                    }


                } // for (auto &out_k_idx: found_mine_outputs)


                // once tx and outputs were added, update Accounts table

                XmrAccount updated_acc = *acc;

                updated_acc.total_received = acc->total_received + tx_data.total_received;

                if (xmr_accounts->update(*acc, updated_acc))
                {
                    // if success, set acc to updated_acc;
                    *acc = updated_acc;
                }

            } // if (!found_mine_outputs.empty())


            // SECOND component: Checking for our key images, i.e., inputs.

            // no need mutex here, as this will be exectued only after
            // the above. there is no threads here.
            oi_identification.identify_inputs(known_outputs_keys);


            if (!oi_identification.identified_inputs.empty())
            {
                // some inputs were identified as ours in a given tx.
                // so now, go over those inputs, and check if
                // get detail info for each found mixin output from database

                vector<XmrInput> inputs_found;

                for (auto& in_info: oi_identification.identified_inputs)
                {
                    XmrOutput out;

                    if (xmr_accounts->output_exists(in_info.out_pub_key, out))
                    {
                        cout << "input uses some mixins which are our outputs"
                             << out << endl;

                        // seems that this key image is ours.
                        // so get it infromatoin from database into XmrInput
                        // database structure that will be written later
                        // on into database.

                        XmrInput in_data;

                        in_data.account_id  = acc->id;
                        in_data.tx_id       = 0; // for now zero, later we set it
                        in_data.output_id   = out.id;
                        in_data.key_image   = in_info.key_img;
                        in_data.amount      = out.amount; // must match corresponding output's amount
                        in_data.timestamp   = blk_timestamp_mysql_format;

                        inputs_found.push_back(in_data);

                        // a key image has only one real mixin. Rest is fake.
                        // so if we find a candidate, break the search.

                        // break;

                    } // if (xmr_accounts->output_exists(output_public_key_str, out))

                } // for (auto& in_info: oi_identification.identified_inputs)


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

                        tx_data.hash           = oi_identification.tx_hash_str;
                        tx_data.prefix_hash    = oi_identification.tx_prefix_hash_str;
                        tx_data.account_id     = acc->id;
                        tx_data.total_received = 0; // because this is spending, total_recieved is 0
                        tx_data.total_sent     = total_sent;
                        tx_data.unlock_time    = tx.unlock_time;
                        tx_data.height         = searched_blk_no;
                        tx_data.coinbase       = oi_identification.tx_is_coinbase;
                        tx_data.is_rct         = oi_identification.is_rct;
                        tx_data.rct_type       = oi_identification.rct_type;
                        tx_data.spendable      = is_spendable;
                        tx_data.payment_id     = CurrentBlockchainStatus::get_payment_id_as_string(tx);
                        tx_data.mixin          = get_mixin_no(tx) - 1;
                        tx_data.timestamp      = blk_timestamp_mysql_format;

                        // insert tx_data into mysql's Transactions table
                        tx_mysql_id = xmr_accounts->insert_tx(tx_data);

                        if (tx_mysql_id == 0)
                        {
                            //cerr << "tx_mysql_id is zero!" << endl;
                            //throw TxSearchException("tx_mysql_id is zero!");
                        }

                    } //   if (tx_mysql_id == 0)

                    // save all input found into database
                    for (XmrInput& in_data: inputs_found)
                    {
                        in_data.tx_id = tx_mysql_id; // set tx id now. before we made it 0
                        uint64_t in_mysql_id = xmr_accounts->insert_input(in_data);
                    }

                } //  if (!inputs_found.empty())


            } //  if (!oi_identification.identified_inputs.empty())


        } // for (const transaction& tx: blk_txs)


        if ((loop_timestamp - current_timestamp > UPDATE_SCANNED_HEIGHT_INTERVAL)
            || searched_blk_no == CurrentBlockchainStatus::current_height)
        {
            // update scanned_block_height every given interval
            // or when we reached top of the blockchain

            XmrAccount updated_acc = *acc;

            updated_acc.scanned_block_height = searched_blk_no;

            if (xmr_accounts->update(*acc, updated_acc))
            {
                // iff success, set acc to updated_acc;
                cout << "scanned_block_height updated"  << endl;
                *acc = updated_acc;
            }

            current_timestamp = loop_timestamp;
        }

        ++searched_blk_no;

    } // while(continue_search)
}

void
TxSearch::stop()
{
    cout << "Stoping the thread by setting continue_search=false" << endl;
    continue_search = false;
}

TxSearch::~TxSearch()
{
    cout << "TxSearch destroyed" << endl;
}

void
TxSearch::set_searched_blk_no(uint64_t new_value)
{
    searched_blk_no = new_value;
}


void
TxSearch::ping()
{
    cout << "new last_ping_timestamp: " << last_ping_timestamp << endl;

    last_ping_timestamp = chrono::duration_cast<chrono::seconds>(
            chrono::system_clock::now().time_since_epoch()).count();
}

bool
TxSearch::still_searching()
{
    return continue_search;
}

void
TxSearch::populate_known_outputs()
{
    vector<XmrOutput> outs;

    if (xmr_accounts->select_outputs(acc->id, outs))
    {
        for (const XmrOutput& out: outs)
        {
            known_outputs_keys.push_back(make_pair(out.out_pub_key, out.amount));
        }
    }
}


json
TxSearch::find_txs_in_mempool(
        vector<pair<uint64_t, transaction>> mempool_txs)
{
    json j_transactions = json::array();

    uint64_t current_height = CurrentBlockchainStatus::get_current_blockchain_height();

    vector<pair<string, uint64_t>> known_outputs_keys_copy;

    {
        // copy known ouputs. mutex is needed as known_outputs_keys can
        // be updated at the same time as used in this here.
        // we make its copy as to keep mutex for short time,
        // and read only on copy safely.
        std::lock_guard<std::mutex> lck (getting_known_outputs_keys);
        known_outputs_keys_copy = known_outputs_keys;
    }

    for (const pair<uint64_t, transaction>& mtx: mempool_txs)
    {

        uint64_t recieve_time = mtx.first;

        const transaction& tx = mtx.second;

        // Class that is resposnible for idenficitaction of our outputs
        // and inputs in a given tx.
        OutputInputIdentification oi_identification {&address, &viewkey, &tx};

        // FIRSt step. to search for the incoming xmr, we use address, viewkey and
        // outputs public key.
        oi_identification.identify_outputs();

        //vector<uint64_t> amount_specific_indices;

        // if we identified some outputs as ours,
        // save them into json to be returned.
        if (!oi_identification.identified_outputs.empty())
        {
            json j_tx;

            j_tx["id"]             = 0; // dont have any database id for tx in mempool
                                        // this id is used for sorting txs in the frontend.

            j_tx["hash"]           = oi_identification.tx_hash_str;
            j_tx["timestamp"]      = timestamp_to_str(recieve_time); // when it got into mempool
            j_tx["total_received"] = oi_identification.total_received;
            j_tx["total_sent"]     = 0; // to be set later when looking for key images
            j_tx["unlock_time"]    = 0; // for mempool we set it to zero
                                        // since we dont have block_height to work with
            j_tx["height"]         = current_height; // put current blockchain height,
                                    // just to indicate to frontend that this
                                    // tx is younger than 10 blocks so that
                                    // it shows unconfirmed message.
            j_tx["payment_id"]     = CurrentBlockchainStatus::get_payment_id_as_string(tx);
            j_tx["coinbase"]       = false; // mempool tx are not coinbase, so always false
            j_tx["is_rct"]         = oi_identification.is_rct;
            j_tx["rct_type"]       = oi_identification.rct_type;
            j_tx["mixin"]          = get_mixin_no(tx) - 1;
            j_tx["mempool"]        = true;

            j_transactions.push_back(j_tx);
        }


        // SECOND step: Checking for our key images, i.e., inputs.

        // no need mutex here, as this will be exectued only after
        // the above. there is no threads here.
        oi_identification.identify_inputs(known_outputs_keys_copy);

        if (!oi_identification.identified_inputs.empty())
        {
            // if we find something we need to construct spent_outputs json array
            // that will be appended into j_tx above. or in case this is
            // only spending tx, i.e., no outputs were found, we need to custruct
            // new j_tx.

            json spend_keys;
            uint64_t total_sent {0};

            for (auto& in_info: oi_identification.identified_inputs)
            {
                // need to get output info from mysql, as we need
                // to know output's amount, its orginal
                // tx public key and its index in that tx
                XmrOutput out;

                if (xmr_accounts->output_exists(in_info.out_pub_key, out))
                {
                    uint64_t output_amount = out.amount;
                    string tx_pub_key = out.tx_pub_key;
                    uint64_t out_index  = out.out_index;
                    uint64_t mixin = out.mixin; // mixin not used but get it anyway
                                                // as in mymonero

                    total_sent += output_amount;

                    spend_keys.push_back({
                          {"key_image" , in_info.key_img},
                          {"amount"    , output_amount},
                          {"tx_pub_key", tx_pub_key},
                          {"out_index" , out_index},
                          {"mixin"     , mixin},
                    });
                }
            }


            if (!spend_keys.empty())
            {
                // check if we got outputs in this tx. if yes
                // we use exising j_tx. If not, we need to construct new
                // j_tx object.

                if (!oi_identification.identified_outputs.empty())
                {
                    // we have outputs in this tx as well, so use
                    // exisiting j_tx. we add spending info
                    // to j_tx created before.

                    json& j_tx = j_transactions.back();

                    j_tx["total_sent"]    = total_sent;
                    j_tx["spent_outputs"] = spend_keys;
                }
                else
                {
                    // we dont have any outputs in this tx as
                    // this is spend only tx, so we need to create new
                    // j_tx.

                    json j_tx;

                    j_tx["id"]             = 0; // dont have any database id for tx in mempool
                                               // this id is used for sorting txs in the frontend.

                    j_tx["hash"]           = oi_identification.tx_hash_str;
                    j_tx["timestamp"]      = timestamp_to_str(recieve_time); // when it got into mempool
                    j_tx["total_received"] = 0; // we did not recive any outputs/xmr
                    j_tx["total_sent"]     = total_sent; // to be set later when looking for key images
                    j_tx["unlock_time"]    = 0;          // for mempool we set it to zero
                                                         // since we dont have block_height to work with
                    j_tx["height"]         = current_height; // put current blockchain height,
                                                        // just to indicate to frontend that this
                                                        // tx is younger than 10 blocks so that
                                                        // it shows unconfirmed message.
                    j_tx["payment_id"]     = CurrentBlockchainStatus::get_payment_id_as_string(tx);
                    j_tx["coinbase"]       = false; // mempool tx are not coinbase, so always false
                    j_tx["is_rct"]         = oi_identification.is_rct;
                    j_tx["rct_type"]       = oi_identification.rct_type;
                    j_tx["mixin"]          = get_mixin_no(tx) - 1;
                    j_tx["mempool"]        = true;
                    j_tx["spent_outputs"]  = spend_keys;

                    j_transactions.push_back(j_tx);

                } // else of if (!oi_identification.identified_outputs.empty())

            } //  if (!spend_keys.empty())

        } // if (!oi_identification.identified_inputs.empty())

    } // for (const transaction& tx: txs_to_check)

    return j_transactions;

}




    pair<account_public_address, secret_key>
TxSearch::get_xmr_address_viewkey() const
{
    return make_pair(address, viewkey);
}

}