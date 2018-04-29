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

    // creates an mysql connection for this thread
    xmr_accounts = make_shared<MySqlAccounts>();

    network_type net_type = CurrentBlockchainStatus::net_type;

    if (!xmreg::parse_str_address(acc->address, address, net_type))
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

    last_ping_timestamp = 0;

    ping();
}

void
TxSearch::search()
{

    uint64_t current_timestamp = chrono::duration_cast<chrono::seconds>(
            chrono::system_clock::now().time_since_epoch()).count();

    last_ping_timestamp = current_timestamp;


    uint64_t loop_idx {0};

    // we put everything in massive catch, as there are plenty ways in which
    // an exceptions can be thrown here. Mostly from mysql.
    // but because this is detatch thread, we cant catch them in main thread.
    // program will blow up if exception is thrown. need to handle exceptions
    // here.
    try
    {
        while(continue_search)
        {
            ++loop_idx;

            uint64_t loop_timestamp {current_timestamp};

            if (loop_idx % 10 == 0)
            {
                // get loop time every fith iteration. no need to call it
                // all the time.
                loop_timestamp = chrono::duration_cast<chrono::seconds>(
                        chrono::system_clock::now().time_since_epoch()).count();

                if (loop_timestamp - last_ping_timestamp > thread_search_life)
                {

                    // also check if we caught up with current blockchain height
                    if (searched_blk_no >= CurrentBlockchainStatus::current_height)
                    {
                        stop();
                        continue;
                    }
                }
            }

            if (searched_blk_no > CurrentBlockchainStatus::current_height)
            {
                fmt::print("searched_blk_no {:d} and current_height {:d}\n",
                           searched_blk_no, CurrentBlockchainStatus::current_height);

                std::this_thread::sleep_for(
                        std::chrono::seconds(
                                CurrentBlockchainStatus::refresh_block_status_every_seconds)
                );

                continue;
            }

            // get block cointaining this tx
            block blk;

            if (!CurrentBlockchainStatus::get_block(searched_blk_no, blk))
            {
                cerr << "Cant get block of height: " + to_string(searched_blk_no) << endl;

                // update current_height of blockchain, as maybe top block(s)
                // were dropped due to reorganization.
                CurrentBlockchainStatus::update_current_blockchain_height();

                // if any txs that we already indexed got orphaned as a consequence of this
                // MySqlAccounts::select_txs_for_account_spendability_check should
                // update database accordingly when get_address_txs is executed.

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
                // print status every 100th block

                cout << " - searching block " << searched_blk_no
                     << " of hash: "
                     << searched_blk_no << get_block_hash(blk) << '\n';
            }

            // we will only create mysql DateTime object once, anything is found
            // in a given block;
            unique_ptr<DateTime> blk_timestamp_mysql_format;

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
                // Class that is responsible for identification of our outputs
                // and inputs in a given tx.
                OutputInputIdentification oi_identification {&address, &viewkey, &tx};

                // flag indicating whether the txs in the given block are spendable.
                // this is true when block number is more than 10 blocks from current
                // blockchain height.

                bool is_spendable = CurrentBlockchainStatus::is_tx_unlocked(
                        tx.unlock_time, searched_blk_no);


                // this is id of txs in lmdb blockchain table.
                // it will be used mostly to sort txs in the frontend.
                uint64_t blockchain_tx_id {0};

                if (!CurrentBlockchainStatus::tx_exist(oi_identification.tx_hash, blockchain_tx_id))
                {
                    cerr << "Tx " << oi_identification.get_tx_hash_str()
                         << "not found in blockchain !" << '\n';
                    continue;
                }

                // FIRSt step.
                oi_identification.identify_outputs();

                vector<uint64_t> amount_specific_indices;

                uint64_t tx_mysql_id {0};

                // create pointer to mysql transaction object
                // that we will initilize if we find something.
                unique_ptr<mysqlpp::Transaction> mysql_transaction;

                // if we identified some outputs as ours,
                // save them into mysql.
                if (!oi_identification.identified_outputs.empty())
                {
//                     before adding this tx and its outputs to mysql
//                     check if it already exists. So that we dont
//                     do it twice.
//
//                     2018:02:01 Dont know why I added this before?
//                     this results in incorrect balances in some cases
//                     as it removes already existing tx data? Dont know
//                     why it was added.
//                     Maybe I added it to enable rescanning blockchain? Thus
//                     it would be deleting already exisitng tx when rescanning
//                     blockchain
//
//                    XmrTransaction tx_data_existing;
//
//                    if (xmr_accounts->tx_exists(acc->id,
//                                                oi_identification.tx_hash_str,
//                                                tx_data_existing))
//                    {
//                        cout << "\nTransaction " << oi_identification.tx_hash_str
//                             << " already present in mysql"
//                             << endl;
//
//                        // if tx is already present for that user,
//                        // we remove it, as we get it data from scrach
//
//                        if (xmr_accounts->delete_tx(tx_data_existing.id) == 0)
//                        {
//                            string msg = fmt::format("xmr_accounts->delete_tx(%d)",
//                                                     tx_data_existing.id);
//                            cerr << msg << endl;
//                            throw TxSearchException(msg);
//                        }
//                    }

                    if (!blk_timestamp_mysql_format)
                    {
                        blk_timestamp_mysql_format
                                = unique_ptr<DateTime>(
                                        new DateTime(static_cast<time_t>(blk.timestamp)));
                    }

                    if (!mysql_transaction)
                    {
                        // start mysql transaction here
                        mysql_transaction
                                = unique_ptr<mysqlpp::Transaction>(
                                           new mysqlpp::Transaction(
                                                   xmr_accounts->get_connection()
                                                           ->get_connection()));
                    }


                    XmrTransaction tx_data;

                    tx_data.hash             = oi_identification.get_tx_hash_str();
                    tx_data.prefix_hash      = oi_identification.get_tx_prefix_hash_str();
                    tx_data.tx_pub_key       = oi_identification.get_tx_pub_key_str();
                    tx_data.account_id       = acc->id;
                    tx_data.blockchain_tx_id = blockchain_tx_id;
                    tx_data.total_received   = oi_identification.total_received;
                    tx_data.total_sent       = 0; // at this stage we don't have any
                                                 // info about spendings

                                                 // this is current block + unlock time
                                                 // for regular tx, the unlock time is
                                                 // default of 10 blocks.
                                                 // for coinbase tx it is 60 blocks
                    tx_data.unlock_time      = tx.unlock_time;

                    tx_data.height           = searched_blk_no;
                    tx_data.coinbase         = oi_identification.tx_is_coinbase;
                    tx_data.is_rct           = oi_identification.is_rct;
                    tx_data.rct_type         = oi_identification.rct_type;
                    tx_data.spendable        = is_spendable;
                    tx_data.payment_id       = CurrentBlockchainStatus::get_payment_id_as_string(tx);
                    tx_data.mixin            = oi_identification.get_mixin_no();
                    tx_data.timestamp        = *blk_timestamp_mysql_format;


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
                        //todo what should be done when insert_tx fails?
                    }

                    // now add the found outputs into Outputs tables
                    for (auto& out_info: oi_identification.identified_outputs)
                    {
                        XmrOutput out_data;

                        out_data.account_id   = acc->id;
                        out_data.tx_id        = tx_mysql_id;
                        out_data.out_pub_key  = out_info.pub_key;
                        out_data.tx_pub_key   = oi_identification.get_tx_pub_key_str();
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
                            //todo what should be done when insert_tx fails?
                        }

                        {
                            std::lock_guard<std::mutex> lck (getting_known_outputs_keys);
                            known_outputs_keys.push_back(make_pair(out_info.pub_key, out_info.amount));
                        }

                    } //  for (auto& out_info: oi_identification.identified_outputs)

                } // if (!found_mine_outputs.empty())


                // SECOND component: Checking for our key images, i.e., inputs.

                // no need mutex here, as this will be exectued only after
                // the above. there is no threads here.
                oi_identification.identify_inputs(known_outputs_keys);


                if (!oi_identification.identified_inputs.empty())
                {
                    // some inputs were identified as ours in a given tx.
                    // so now, go over those inputs, and check
                    // get detail info for each found mixin output from database


                    if (!blk_timestamp_mysql_format)
                    {
                        blk_timestamp_mysql_format
                                = unique_ptr<DateTime>(
                                new DateTime(static_cast<time_t>(blk.timestamp)));
                    }

                    if (!mysql_transaction)
                    {
                        // start mysql transaction here if not already present
                        mysql_transaction
                                = unique_ptr<mysqlpp::Transaction>(
                                new mysqlpp::Transaction(
                                        xmr_accounts->get_connection()
                                                ->get_connection()));

                    }


                    vector<XmrInput> inputs_found;

                    for (auto& in_info: oi_identification.identified_inputs)
                    {
                        XmrOutput out;

                        if (xmr_accounts->output_exists(in_info.out_pub_key, out))
                        {
                            cout << "input uses some mixins which are our outputs"
                                 << out << '\n';

                            // seems that this key image is ours.
                            // so get it information from database into XmrInput
                            // database structure that will be written later
                            // on into database.

                            XmrInput in_data;

                            in_data.account_id  = acc->id;
                            in_data.tx_id       = 0; // for now zero, later we set it
                            in_data.output_id   = out.id;
                            in_data.key_image   = in_info.key_img;
                            in_data.amount      = out.amount; // must match corresponding output's amount
                            in_data.timestamp   = *blk_timestamp_mysql_format;

                            inputs_found.push_back(in_data);

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
                            total_sent += in_data.amount;

                        if (tx_mysql_id == 0)
                        {
                            // this txs hasnt been seen in step first.
                            // it means that it only contains potentially our
                            // key images. It does not have our outputs.
                            // so write it to mysql as ours, with
                            // total received of 0.

                            XmrTransaction tx_data;

                            tx_data.hash             = oi_identification.get_tx_hash_str();
                            tx_data.prefix_hash      = oi_identification.get_tx_prefix_hash_str();
                            tx_data.tx_pub_key       = oi_identification.get_tx_pub_key_str();
                            tx_data.account_id       = acc->id;
                            tx_data.blockchain_tx_id = blockchain_tx_id;
                            tx_data.total_received   = 0; // because this is spending, total_recieved is 0
                            tx_data.total_sent       = total_sent;
                            tx_data.unlock_time      = tx.unlock_time;
                            tx_data.height           = searched_blk_no;
                            tx_data.coinbase         = oi_identification.tx_is_coinbase;
                            tx_data.is_rct           = oi_identification.is_rct;
                            tx_data.rct_type         = oi_identification.rct_type;
                            tx_data.spendable        = is_spendable;
                            tx_data.payment_id       = CurrentBlockchainStatus::get_payment_id_as_string(tx);
                            tx_data.mixin            = oi_identification.get_mixin_no();
                            tx_data.timestamp        = *blk_timestamp_mysql_format;

                            // insert tx_data into mysql's Transactions table
                            tx_mysql_id = xmr_accounts->insert_tx(tx_data);

                            if (tx_mysql_id == 0)
                            {
                                //cerr << "tx_mysql_id is zero!" << endl;
                                //throw TxSearchException("tx_mysql_id is zero!");
                                // it did not insert this tx, because maybe it already
                                // exisits in the MySQL. So maybe can now
                                // check if we have it and get tx_mysql_id this way.
                                //todo what should be done when insert_tx fails?
                            }

                        } //   if (tx_mysql_id == 0)

                        // save all input found into database
                        for (XmrInput& in_data: inputs_found)
                        {
                            in_data.tx_id = tx_mysql_id; // set tx id now. before we made it 0

                            uint64_t in_mysql_id = xmr_accounts->insert_input(in_data);

                            //todo what shoud we do when insert_input fails?
                        }

                    } //  if (!inputs_found.empty())

                } //  if (!oi_identification.identified_inputs.empty())

                // if we get to this point, we assume that all tx related tables are ready
                // to be written, i.e., Transactions, Outputs and Inputs. If so, write
                // all this into database.

                if (mysql_transaction)
                    mysql_transaction->commit();

            } // for (const transaction& tx: blk_txs)


            if ((loop_timestamp - current_timestamp > UPDATE_SCANNED_HEIGHT_INTERVAL)
                || searched_blk_no == CurrentBlockchainStatus::current_height)
            {
                // update scanned_block_height every given interval
                // or when we reached top of the blockchain

                XmrAccount updated_acc = *acc;

                if (!blk_timestamp_mysql_format)
                {
                    blk_timestamp_mysql_format
                            = unique_ptr<DateTime>(
                            new DateTime(static_cast<time_t>(blk.timestamp)));
                }

                updated_acc.scanned_block_height    = searched_blk_no;
                updated_acc.scanned_block_timestamp = *blk_timestamp_mysql_format;

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
    catch(TxSearchException const& e)
    {
        cerr << "TxSearchException in TxSearch: " << e.what() << " for " << acc->address << '\n';
    }
    catch(mysqlpp::Exception const& e)
    {
        cerr << "mysqlpp::Exception in TxSearch: " << e.what() << " for " << acc->address << '\n';
    }
    catch(std::exception const& e)
    {
        cerr << "std::exception in TxSearch: " << e.what() << " for " << acc->address << '\n';
    }
    catch(...)
    {
        cerr << "Unknown exception in TxSearch for " << acc->address << '\n';
    }
}

void
TxSearch::stop()
{
    cout << "Stopping the thread by setting continue_search=false" << endl;
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

uint64_t
TxSearch::get_searched_blk_no() const
{
    return searched_blk_no;
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

vector<pair<string, uint64_t>>
TxSearch::get_known_outputs_keys()
{
    std::lock_guard<std::mutex> lck (getting_known_outputs_keys);
    return known_outputs_keys;
};

json
TxSearch::find_txs_in_mempool(
        vector<pair<uint64_t, transaction>> mempool_txs)
{
    json j_transactions = json::array();

    uint64_t current_height = CurrentBlockchainStatus::get_current_blockchain_height();

    vector<pair<string, uint64_t>> known_outputs_keys_copy = get_known_outputs_keys();

    // since find_txs_in_mempool can be called outside of this thread,
    // we need to use local connection. we cant use connection that the
    // main search method is using, as we can end up wtih mysql errors
    // mysql will blow up when two queries are done at the same
    // time in a single connection.
    // so we create local connection here, only to be used in this method.

    shared_ptr<MySqlAccounts> local_xmr_accounts = make_shared<MySqlAccounts>();

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

            j_tx["hash"]           = oi_identification.get_tx_hash_str();
            j_tx["tx_pub_key"]     = oi_identification.get_tx_pub_key_str();
            j_tx["timestamp"]      = recieve_time; // when it got into mempool
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
            j_tx["mixin"]          = oi_identification.get_mixin_no();
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

                if (local_xmr_accounts->output_exists(in_info.out_pub_key, out))
                {
                    total_sent += out.amount;

                    spend_keys.push_back({
                          {"key_image" , in_info.key_img},
                          {"amount"    , out.amount},
                          {"tx_pub_key", out.tx_pub_key},
                          {"out_index" , out.out_index},
                          {"mixin"     , out.mixin},
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

                    j_tx["id"]             = 0;          // dont have any database id for tx in mempool
                                                         // this id is used for sorting txs in the frontend.

                    j_tx["hash"]           = oi_identification.get_tx_hash_str();
                    j_tx["tx_pub_key"]     = oi_identification.get_tx_pub_key_str();
                    j_tx["timestamp"]      = recieve_time; // when it got into mempool
                    j_tx["total_received"] = 0;          // we did not recive any outputs/xmr
                    j_tx["total_sent"]     = total_sent; // to be set later when looking for key images
                    j_tx["unlock_time"]    = 0;          // for mempool we set it to zero
                                                         // since we dont have block_height to work with
                    j_tx["height"]         = current_height; // put current blockchain height,
                                                        // just to indicate to frontend that this
                                                        // tx is younger than 10 blocks so that
                                                        // it shows unconfirmed message.
                    j_tx["payment_id"]     = CurrentBlockchainStatus::get_payment_id_as_string(tx);
                    j_tx["coinbase"]       = false;     // mempool tx are not coinbase, so always false
                    j_tx["is_rct"]         = oi_identification.is_rct;
                    j_tx["rct_type"]       = oi_identification.rct_type;
                    j_tx["mixin"]          = get_mixin_no(tx);
                    j_tx["mempool"]        = true;
                    j_tx["spent_outputs"]  = spend_keys;

                    j_transactions.push_back(j_tx);

                } // else of if (!oi_identification.identified_outputs.empty())

            } //  if (!spend_keys.empty())

        } // if (!oi_identification.identified_inputs.empty())

    } // for (const transaction& tx: txs_to_check)

    return j_transactions;

}


pair<address_parse_info, secret_key>
TxSearch::get_xmr_address_viewkey() const
{
    return make_pair(address, viewkey);
}


void
TxSearch::set_search_thread_life(uint64_t life_seconds)
{
    thread_search_life = life_seconds;
}

// default value of static veriables
uint64_t TxSearch::thread_search_life {600};

}
