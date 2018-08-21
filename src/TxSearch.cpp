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

TxSearch::TxSearch(XmrAccount& _acc,
                   std::shared_ptr<CurrentBlockchainStatus> _current_bc_status)
    : current_bc_status {_current_bc_status}
{
    acc = make_shared<XmrAccount>(_acc);

    // creates an mysql connection for this thread
    xmr_accounts = make_shared<MySqlAccounts>(current_bc_status);

    network_type net_type = current_bc_status->get_bc_setup().net_type;


    if (!xmreg::parse_str_address(acc->address, address, net_type))
    {
        OMERROR << "Cant parse address: " + acc->address;
        throw TxSearchException("Cant parse address: " + acc->address);
    }

    if (!xmreg::parse_str_secret_key(acc->viewkey, viewkey))
    {
        OMERROR << "Cant parse private view key: " + acc->address;
        throw TxSearchException("Cant parse private view key: "
                                + acc->viewkey);
    }

    populate_known_outputs();

    // start searching from last block that we searched for
    // this accont
    searched_blk_no = acc->scanned_block_height;

    last_ping_timestamp = 0;

    ping();
}

void
TxSearch::operator()()
{

    uint64_t current_timestamp = get_current_timestamp();

    last_ping_timestamp = current_timestamp;

    uint64_t blocks_lookahead
            = current_bc_status->get_bc_setup().blocks_search_lookahead;

    // we put everything in massive catch, as there are plenty ways in which
    // an exceptions can be thrown here. Mostly from mysql.
    // but because this is detatch thread, we cant catch them in main thread.
    // program will blow up if exception is thrown. need to handle exceptions
    // here.
    try
    {
        while(continue_search)
        {
            uint64_t loop_timestamp {current_timestamp};

            uint64_t last_block_height = current_bc_status->current_height;

            uint64_t h1 = searched_blk_no;
            uint64_t h2 = std::min(h1 + blocks_lookahead - 1, last_block_height);

            vector<block> blocks = current_bc_status->get_blocks_range(h1, h2);

            if (blocks.empty())
            {
                OMINFO << "Cant get blocks from " << h1
                       << " to " << h2;

                std::this_thread::sleep_for(
                        std::chrono::seconds(
                                current_bc_status->get_bc_setup()
                                .refresh_block_status_every_seconds)
                );

                loop_timestamp = get_current_timestamp();

                // if thread has lived longer than thread_search_li
                // without last_ping_timestamp being updated,
                // stop the thread
                if (loop_timestamp - last_ping_timestamp > thread_search_life)
                {
                    OMINFO << "Search thread stopped for address "
                           << acc->address;
                    stop();
                }

                // update current_height of blockchain, as maybe top block(s)
                // were dropped due to reorganization.
                //CurrentBlockchainStatus::update_current_blockchain_height();

                // if any txs that we already indexed got orphaned as a
                // consequence of this
                // MySqlAccounts::select_txs_for_account_spendability_check
                // should
                // update database accordingly when get_address_txs is executed.

                continue;
            }

            OMINFO << "Analyzing " << blocks.size() << " blocks from "
                   << h1 << " to " << h2
                   << " out of " << last_block_height << " blocks";

            vector<crypto::hash> txs_hashes_from_blocks;
            vector<transaction> txs_in_blocks;
            vector<CurrentBlockchainStatus::txs_tuple_t> txs_data;

            if (!current_bc_status->get_txs_in_blocks(blocks,
                                                      txs_hashes_from_blocks,
                                                      txs_in_blocks,
                                                      txs_data))
            {
                OMERROR << "Cant get tx in blocks from " << h1 << " to " << h2;;
                return;
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
            // then we assume its our key image (i.e. we spend this output).
            // Off course this is only
            // assumption as our outputs can be used in key images of others for their
            // mixin purposes. Thus, we sent to the front end the list of key images
            // that we think are yours, and the frontend, because it has spend key,
            // can filter out false positives.

            size_t tx_idx {0};

            for (auto const& tx_tuple: txs_data)
            {
                crypto::hash const& tx_hash = txs_hashes_from_blocks[tx_idx];
                transaction const& tx       = txs_in_blocks[tx_idx++];
                uint64_t blk_height         = std::get<0>(tx_tuple);
                uint64_t blk_timestamp      = std::get<1>(tx_tuple);
                bool is_coinbase            = std::get<2>(tx_tuple);

                //cout << "\n\n\n" << blk_height << '\n';

                // Class that is responsible for identification of our outputs
                // and inputs in a given tx.
                OutputInputIdentification oi_identification {
                            &address, &viewkey, &tx, tx_hash,
                            is_coinbase, current_bc_status};

                // flag indicating whether the txs in the given block are spendable.
                // this is true when block number is more than 10 blocks from current
                // blockchain height.

                bool is_spendable = current_bc_status->is_tx_unlocked(
                        tx.unlock_time, blk_height);


                // this is id of txs in lmdb blockchain table.
                // it will be used mostly to sort txs in the frontend.
                uint64_t blockchain_tx_id {0};

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

                    if (!blk_timestamp_mysql_format)
                    {
                        blk_timestamp_mysql_format
                                = unique_ptr<DateTime>(
                                        new DateTime(
                                        static_cast<time_t>(blk_timestamp)));
                    }

                    if (!mysql_transaction)
                    {
                        // start mysql transaction here
                        mysql_transaction
                                = unique_ptr<mysqlpp::Transaction>(
                                           new mysqlpp::Transaction(
                                                   xmr_accounts->get_connection()
                                                           ->get_connection()));

                        // when we rescan blockchain some txs can already
                        // be present in the mysql. So remove them, and their
                        // associated data in that case to repopulate fresh
                        // tx data
                        if (!delete_existing_tx_if_exists(
                                    oi_identification.get_tx_hash_str()))
                            throw TxSearchException("Cant delete tx "
                                         + oi_identification.tx_hash_str);

                    }


                    if (!current_bc_status->tx_exist(tx_hash, blockchain_tx_id))
                    {
                        OMERROR << "Tx " << oi_identification.get_tx_hash_str()
                                << " " << pod_to_hex(tx_hash)
                                << " not found in blockchain !";
                        throw TxSearchException("Cant get tx from blockchain: "
                                                + pod_to_hex(tx_hash));
                    }

                    OMINFO << " - found some outputs in block " << blk_height
                            << ", tx: " << oi_identification.get_tx_hash_str();


                    XmrTransaction tx_data;

                    tx_data.id               = mysqlpp::null;
                    tx_data.hash             = oi_identification
                                                  .get_tx_hash_str();
                    tx_data.prefix_hash      = oi_identification
                                                  .get_tx_prefix_hash_str();
                    tx_data.tx_pub_key       = oi_identification
                                                 .get_tx_pub_key_str();
                    tx_data.account_id       = acc->id.data;
                    tx_data.blockchain_tx_id = blockchain_tx_id;
                    tx_data.total_received   = oi_identification.total_received;
                    tx_data.total_sent       = 0; // at this stage we don't have
                                                 //  anyinfo about spendings

                                                 // this is current block
                                                 // + unlock time
                                                 // for regular tx, the unlock time is
                                                 // default of 10 blocks.
                                                 // for coinbase tx it is 60 blocks
                    tx_data.unlock_time      = tx.unlock_time;

                    tx_data.height           = blk_height;
                    tx_data.coinbase         = oi_identification.tx_is_coinbase;
                    tx_data.is_rct           = oi_identification.is_rct;
                    tx_data.rct_type         = oi_identification.rct_type;
                    tx_data.spendable        = is_spendable;
                    tx_data.payment_id       = current_bc_status
                                                ->get_payment_id_as_string(tx);
                    tx_data.mixin            = oi_identification.get_mixin_no();
                    tx_data.timestamp        = *blk_timestamp_mysql_format;


                    // insert tx_data into mysql's Transactions table
                    tx_mysql_id = xmr_accounts->insert(tx_data);

                    // get amount specific (i.e., global) indices of outputs
                    if (!current_bc_status->get_amount_specific_indices(
                            tx_hash, amount_specific_indices))
                    {
                        OMERROR << "cant get_amount_specific_indices!";
                        throw TxSearchException(
                                    "cant get_amount_specific_indices!");
                    }

                    if (tx_mysql_id == 0)
                    {                        
                        OMERROR << "tx_mysql_id is zero!" << tx_data;
                        throw TxSearchException("tx_mysql_id is zero!");                        
                    }

                    vector<XmrOutput> outputs_found;

                    // now add the found outputs into Outputs tables
                    for (auto& out_info: oi_identification.identified_outputs)
                    {
                        XmrOutput out_data;

                        out_data.id           = mysqlpp::null;
                        out_data.account_id   = acc->id.data;
                        out_data.tx_id        = tx_mysql_id;
                        out_data.out_pub_key  = pod_to_hex(out_info.pub_key);
                        out_data.tx_pub_key   = oi_identification
                                .get_tx_pub_key_str();
                        out_data.amount       = out_info.amount;
                        out_data.out_index    = out_info.idx_in_tx;
                        out_data.rct_outpk    = out_info.rtc_outpk;
                        out_data.rct_mask     = out_info.rtc_mask;
                        out_data.rct_amount   = out_info.rtc_amount;
                        out_data.global_index = amount_specific_indices
                                .at(out_data.out_index);
                        out_data.mixin        = tx_data.mixin;
                        out_data.timestamp    = tx_data.timestamp;

                        outputs_found.push_back(std::move(out_data));

                        {
                            // add the outputs found into known_outputs_keys map
                            std::lock_guard<std::mutex> lck (
                                        getting_known_outputs_keys);
                            known_outputs_keys.insert(
                                {out_info.pub_key, out_info.amount});
                        }

                    } //  for (auto& out_info: oi_identification.identified_outputs)


                    // insert all outputs found into mysql's outputs table
                    uint64_t no_rows_inserted
                            = xmr_accounts->insert(outputs_found);

                    if (no_rows_inserted == 0)
                    {                        
                        throw TxSearchException("no_rows_inserted is zero!");
                    }

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
                                new DateTime(
                                        static_cast<time_t>(blk_timestamp)));
                    }

                    if (!mysql_transaction)
                    {
                        // start mysql transaction here if not already present
                        mysql_transaction
                                = unique_ptr<mysqlpp::Transaction>(
                                new mysqlpp::Transaction(
                                        xmr_accounts->get_connection()
                                                ->get_connection()));

                        // when we rescan blockchain some txs can already
                        // be present in the mysql. So remove them, and their
                        // associated data in that case to repopulate fresh tx data

                        // this will only execture if no outputs were found
                        // above. So there is no risk of deleting same tx twice
                        if (!delete_existing_tx_if_exists(
                                    oi_identification.get_tx_hash_str()))
                        {
                            throw TxSearchException(
                                    "Cant delete tx "
                                    + oi_identification.tx_hash_str);
                        }
                    }

                    if (blockchain_tx_id == 0)
                    {
                        if (!current_bc_status
                                ->tx_exist(oi_identification.tx_hash,
                                           blockchain_tx_id))
                        {
                            OMERROR << "Tx "
                                    << oi_identification.get_tx_hash_str()
                                     << "not found in blockchain !";
                            throw TxSearchException(
                                        "Cant get tx from blockchain: "
                                        + pod_to_hex(tx_hash));
                        }
                    }

                    OMINFO << "Found some possible inputs in block "
                           << blk_height << ", tx: "
                           << oi_identification.get_tx_hash_str();

                    vector<XmrInput> inputs_found;

                    for (auto& in_info: oi_identification.identified_inputs)
                    {
                        XmrOutput out;

                        if (xmr_accounts->output_exists(
                                    pod_to_hex(in_info.out_pub_key), out))
                        {

                            // seems that this key image is ours.
                            // so get it information from database into XmrInput
                            // database structure that will be written later
                            // on into database.

                            XmrInput in_data;

                            in_data.id          = mysqlpp::null;
                            in_data.account_id  = acc->id.data;
                            in_data.tx_id       = 0; // later we set it
                            in_data.output_id   = out.id.data;
                            in_data.key_image   = in_info.key_img;
                            in_data.amount      = out.amount;
                                                    // must match corresponding
                                                    // output's amount
                            in_data.timestamp   = *blk_timestamp_mysql_format;

                            inputs_found.push_back(in_data);

                        } // if (xmr_accounts->output_exists(o

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

                            tx_data.id               = mysqlpp::null;
                            tx_data.hash             = oi_identification
                                    .get_tx_hash_str();
                            tx_data.prefix_hash      = oi_identification
                                    .get_tx_prefix_hash_str();
                            tx_data.tx_pub_key       = oi_identification
                                    .get_tx_pub_key_str();
                            tx_data.account_id       = acc->id.data;
                            tx_data.blockchain_tx_id = blockchain_tx_id;
                            tx_data.total_received   = 0; // because this is
                                                          //spending,
                                                          //total_recieved is 0
                            tx_data.total_sent       = total_sent;
                            tx_data.unlock_time      = tx.unlock_time;
                            tx_data.height           = blk_height;
                            tx_data.coinbase         = oi_identification
                                    .tx_is_coinbase;
                            tx_data.is_rct           = oi_identification
                                    .is_rct;
                            tx_data.rct_type         = oi_identification
                                    .rct_type;
                            tx_data.spendable        = is_spendable;
                            tx_data.payment_id       = current_bc_status
                                    ->get_payment_id_as_string(tx);
                            tx_data.mixin            = oi_identification
                                    .get_mixin_no();
                            tx_data.timestamp    = *blk_timestamp_mysql_format;

                            // insert tx_data into mysql's Transactions table
                            tx_mysql_id = xmr_accounts->insert(tx_data);

                            if (tx_mysql_id == 0)
                            {
                                OMERROR << "tx_mysql_id is zero!" << tx_data;

                                //cerr << "tx_mysql_id is zero!" << endl;
                                throw TxSearchException("tx_mysql_id is zero!");
                                // it did not insert this tx, because maybe
                                // it already
                                // exisits in the MySQL. So maybe can now
                                // check if we have it and get tx_mysql_id this
                                // way.                               
                            }

                        } //   if (tx_mysql_id == 0)

                        // save all input found into database at once
                        // but first update tx_mysql_id for these inputs
                        for (XmrInput& in_data: inputs_found)
                            in_data.tx_id = tx_mysql_id; // set tx id now.
                                                        //before we made it 0

                        uint64_t no_rows_inserted
                                = xmr_accounts->insert(inputs_found);

                        if (no_rows_inserted == 0)
                        {
                            throw TxSearchException(
                                        "no_rows_inserted is zero!");
                        }

                    } //  if (!inputs_found.empty())

                } //  if (!oi_identification.identified_inputs.empty())

                // if we get to this point, we assume that all tx related
                // tables are ready
                // to be written, i.e., Transactions,
                // Outputs and Inputs. If so, write
                // all this into database.

                if (mysql_transaction)
                    mysql_transaction->commit();

            } // for (auto const& tx_pair: txs_map)

            // update scanned_block_height every given interval
            // or when we reached top of the blockchain

            XmrAccount updated_acc = *acc;

            updated_acc.scanned_block_height    = h2;
            updated_acc.scanned_block_timestamp
                    = DateTime(static_cast<time_t>(
                                   blocks.back().timestamp));

            if (xmr_accounts->update(*acc, updated_acc))
            {
                // iff success, set acc to updated_acc;
                //cout << "scanned_block_height updated\n";
                *acc = updated_acc;
            }

            //current_timestamp = loop_timestamp;

            searched_blk_no = h2 + 1;

        } // while(continue_search)

    }   
    catch(...)
    {
        OMERROR << "Exception in TxSearch for " << acc->address;
        set_exception_ptr();
    }

    // it will stop anyway, but just call it so we get info message pritened out
    stop();
}

void
TxSearch::stop()
{
    OMINFO << "Stopping the thread by setting continue_search=false";
    continue_search = false;
}

TxSearch::~TxSearch()
{
    OMINFO << "TxSearch destroyed";
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

inline uint64_t
TxSearch::get_current_timestamp() const
{
   return  chrono::duration_cast<chrono::seconds>(
          chrono::system_clock::now().time_since_epoch()).count();
}

void
TxSearch::ping()
{
    OMINFO << "New last_ping_timestamp: " << last_ping_timestamp;

    last_ping_timestamp = chrono::duration_cast<chrono::seconds>(
            chrono::system_clock::now().time_since_epoch()).count();
}

bool
TxSearch::still_searching() const
{
    return continue_search;
}

void
TxSearch::populate_known_outputs()
{
    vector<XmrOutput> outs;

    if (xmr_accounts->select(acc->id.data, outs))
    {
        for (const XmrOutput& out: outs)
        {
            public_key out_pub_key;

            hex_to_pod(out.out_pub_key, out_pub_key);

            known_outputs_keys[out_pub_key] = out.amount;
        }
    }
}

TxSearch::known_outputs_t
TxSearch::get_known_outputs_keys()
{
    std::lock_guard<std::mutex> lck (getting_known_outputs_keys);
    return known_outputs_keys;
};

void
TxSearch::find_txs_in_mempool(
        TxSearch::pool_txs_t mempool_txs,
        json* j_transactions)
{   

    *j_transactions = json::array();

    uint64_t current_height = current_bc_status
            ->get_current_blockchain_height();

    known_outputs_t known_outputs_keys_copy = get_known_outputs_keys();

    // since find_txs_in_mempool can be called outside of this thread,
    // we need to use local connection. we cant use connection that the
    // main search method is using, as we can end up wtih mysql errors
    // mysql will blow up when two queries are done at the same
    // time in a single connection.
    // so we create local connection here, only to be used in this method.

    auto local_xmr_accounts = make_shared<MySqlAccounts>(current_bc_status);

    for (auto const& mtx: mempool_txs)
    {

        uint64_t recieve_time = mtx.first;

        const transaction& tx = mtx.second;

        const crypto::hash tx_hash = get_transaction_hash(tx);
        const bool coinbase = is_coinbase(tx);

        // Class that is resposnible for idenficitaction of our outputs
        // and inputs in a given tx.
        OutputInputIdentification oi_identification
                 {&address, &viewkey, &tx, tx_hash, coinbase,
                 current_bc_status};

        // FIRSt step. to search for the incoming xmr, we use address,
        // viewkey and
        // outputs public key.
        oi_identification.identify_outputs();

        //vector<uint64_t> amount_specific_indices;

        // if we identified some outputs as ours,
        // save them into json to be returned.
        if (!oi_identification.identified_outputs.empty())
        {
            json j_tx;

            j_tx["id"]             = 0; // dont have any database id
                                        //for tx in mempool
                                        // this id is used for
                                        // sorting txs in the frontend.

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
            j_tx["payment_id"]     = current_bc_status->get_payment_id_as_string(tx);
            j_tx["coinbase"]       = false; // mempool tx are not coinbase, so always false
            j_tx["is_rct"]         = oi_identification.is_rct;
            j_tx["rct_type"]       = oi_identification.rct_type;
            j_tx["mixin"]          = oi_identification.get_mixin_no();
            j_tx["mempool"]        = true;

            j_transactions->push_back(j_tx);
        }


        // SECOND step: Checking for our key images, i.e., inputs.

        // no need mutex here, as this will be exectued only after
        // the above. there is no threads here.
        oi_identification.identify_inputs(known_outputs_keys_copy);

        if (!oi_identification.identified_inputs.empty())
        {
            // if we find something we need to construct spent_outputs json
            // array that will be appended into j_tx above. or in case
            // this is
            // only spending tx, i.e., no outputs were found, we need to
            // construct new j_tx.

            json spend_keys;
            uint64_t total_sent {0};

            for (auto& in_info: oi_identification.identified_inputs)
            {
                // need to get output info from mysql, as we need
                // to know output's amount, its orginal
                // tx public key and its index in that tx
                XmrOutput out;

                if (local_xmr_accounts->output_exists(
                            pod_to_hex(in_info.out_pub_key), out))
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

                    json& j_tx = j_transactions->back();

                    j_tx["total_sent"]    = total_sent;
                    j_tx["spent_outputs"] = spend_keys;
                }
                else
                {
                    // we dont have any outputs in this tx as
                    // this is spend only tx, so we need to create new
                    // j_tx.

                    json j_tx;

                    j_tx["id"]             = 0; // dont have any database
                                                // id for tx in mempool
                                                // this id is used for
                                                // sorting txs in the
                                                // frontend.

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
                    j_tx["payment_id"]     = current_bc_status
                            ->get_payment_id_as_string(tx);
                    j_tx["coinbase"]       = false;     // mempool tx are not coinbase, so always false
                    j_tx["is_rct"]         = oi_identification.is_rct;
                    j_tx["rct_type"]       = oi_identification.rct_type;
                    j_tx["mixin"]          = get_mixin_no(tx);
                    j_tx["mempool"]        = true;
                    j_tx["spent_outputs"]  = spend_keys;

                    j_transactions->push_back(j_tx);

                } // else of if (!oi_identification.identified_outputs.empty())

            } //  if (!spend_keys.empty())

        } // if (!oi_identification.identified_inputs.empty())

    } // for (const transaction& tx: txs_to_check)
}


TxSearch::addr_view_t
TxSearch::get_xmr_address_viewkey() const
{
    return make_pair(address, viewkey);
}


void
TxSearch::set_search_thread_life(uint64_t life_seconds)
{
    thread_search_life = life_seconds;
}

bool
TxSearch::delete_existing_tx_if_exists(string const& tx_hash)
{
    XmrTransaction tx_data_existing;

    if (xmr_accounts->tx_exists(acc->id.data, tx_hash, tx_data_existing))
    {
        OMINFO << "\nTransaction " << tx_hash
               << " already present in mysql, so remove it";

        // if tx is already present for that user,
        // we remove it, as we get it data from scrach

        if (xmr_accounts->delete_tx(tx_data_existing.id.data) == 0)
        {
            OMERROR << "cant remove tx " << tx_hash;
            return false;
        }
    }

    return true;
}


// default value of static veriables
uint64_t TxSearch::thread_search_life {600};

}
