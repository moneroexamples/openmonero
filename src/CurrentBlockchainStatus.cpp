//
// Created by mwo on 7/01/17.
//

#include "CurrentBlockchainStatus.h"


#include "tools.h"
#include "mylmdb.h"
#include "rpccalls.h"
#include "MySqlAccounts.h"
#include "TxSearch.h"


#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "openmonero"

namespace xmreg
{



// initialize static variables
atomic<uint64_t>        CurrentBlockchainStatus::current_height{0};
string                  CurrentBlockchainStatus::blockchain_path{"/home/mwo/.blockchain/lmdb"};
string                  CurrentBlockchainStatus::deamon_url{"http:://127.0.0.1:18081"};
network_type            CurrentBlockchainStatus::net_type {network_type::MAINNET};
bool                    CurrentBlockchainStatus::do_not_relay{false};
bool                    CurrentBlockchainStatus::is_running{false};
std::thread             CurrentBlockchainStatus::m_thread;
uint64_t                CurrentBlockchainStatus::refresh_block_status_every_seconds{20};
uint64_t                CurrentBlockchainStatus::max_number_of_blocks_to_import{8000};
uint64_t                CurrentBlockchainStatus::search_thread_life_in_seconds {600}; // 10 minutes
vector<pair<uint64_t, transaction>> CurrentBlockchainStatus::mempool_txs;
string                  CurrentBlockchainStatus::import_payment_address_str;
string                  CurrentBlockchainStatus::import_payment_viewkey_str;
uint64_t                CurrentBlockchainStatus::import_fee {10000000000}; // 0.01 xmr
address_parse_info      CurrentBlockchainStatus::import_payment_address;
secret_key              CurrentBlockchainStatus::import_payment_viewkey;
map<string, unique_ptr<TxSearch>> CurrentBlockchainStatus::searching_threads;
cryptonote::Blockchain* CurrentBlockchainStatus::core_storage;
unique_ptr<xmreg::MicroCore>        CurrentBlockchainStatus::mcore;

void
CurrentBlockchainStatus::start_monitor_blockchain_thread()
{
    network_type net_type = CurrentBlockchainStatus::net_type;

    TxSearch::set_search_thread_life(search_thread_life_in_seconds);

    if (!import_payment_address_str.empty() && !import_payment_viewkey_str.empty())
    {
        if (!xmreg::parse_str_address(
                import_payment_address_str,
                import_payment_address,
                CurrentBlockchainStatus::net_type))
        {
            cerr << "Cant parse address_str: "
                 << import_payment_address_str
                 << endl;
            return;
        }

        if (!xmreg::parse_str_secret_key(
                import_payment_viewkey_str,
                import_payment_viewkey))
        {
            cerr << "Cant parse the viewkey_str: "
                 << import_payment_viewkey_str
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
                   update_current_blockchain_height();
                   read_mempool();
                   cout << "Check block height: " << current_height
                        << " no of mempool txs: " << mempool_txs.size();
                   cout << endl;
                   clean_search_thread_map();
                   std::this_thread::sleep_for(
                           std::chrono::seconds(
                                   refresh_block_status_every_seconds));
               }
           }};

        is_running = true;
    }
}


uint64_t
CurrentBlockchainStatus::get_current_blockchain_height()
{

    uint64_t previous_height = current_height;

    try
    {
        return xmreg::MyLMDB::get_blockchain_height(blockchain_path) - 1;
    }
    catch(std::exception& e)
    {
        cerr << "xmreg::MyLMDB::get_blockchain_height: " << e.what() << endl;
        return previous_height;
    }

}


void
CurrentBlockchainStatus::update_current_blockchain_height()
{
    current_height = get_current_blockchain_height();
}

bool
CurrentBlockchainStatus::init_monero_blockchain()
{
    // set  monero log output level
    uint32_t log_level = 0;
    mlog_configure(mlog_get_default_log_path(""), true);

    mcore = unique_ptr<xmreg::MicroCore>(new xmreg::MicroCore{});

    // initialize the core using the blockchain path
    if (!mcore->init(blockchain_path, net_type))
        return false;

    // get the high level Blockchain object to interact
    // with the blockchain lmdb database
    core_storage = &(mcore->get_core());

    return true;
}


bool
CurrentBlockchainStatus::is_tx_unlocked(
        uint64_t unlock_time,
        uint64_t block_height)
{
    if(!is_tx_spendtime_unlocked(unlock_time, block_height))
        return false;

    if(block_height + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE > current_height + 1)
        return false;

    return true;
}


bool
CurrentBlockchainStatus::is_tx_spendtime_unlocked(
        uint64_t unlock_time,
        uint64_t block_height)
{
    if(unlock_time < CRYPTONOTE_MAX_BLOCK_NUMBER)
    {
        //interpret as block index
        if(current_height + CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS >= unlock_time)
            return true;
        else
            return false;
    }
    else
    {
        //interpret as time
        uint64_t current_time = static_cast<uint64_t>(time(NULL));
        // XXX: this needs to be fast, so we'd need to get the starting heights
        // from the daemon to be correct once voting kicks in

        uint64_t v2height = net_type == TESTNET ? 624634 : net_type == STAGENET ? (uint64_t)-1/*TODO*/ : 1009827;

        uint64_t leeway = block_height < v2height
                          ? CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS_V1
                          : CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS_V2;

        if(current_time + leeway >= unlock_time)
            return true;
        else
            return false;
    }

    return false;
}

bool
CurrentBlockchainStatus::get_block(uint64_t height, block &blk)
{
    return mcore->get_block_from_height(height, blk);
}

bool
CurrentBlockchainStatus::get_block_txs(const block &blk, list <transaction> &blk_txs)
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


bool
CurrentBlockchainStatus::tx_exist(const crypto::hash& tx_hash)
{
    return core_storage->have_tx(tx_hash);
}

bool
CurrentBlockchainStatus::tx_exist(const crypto::hash& tx_hash, uint64_t& tx_index)
{
    if (!core_storage->get_db().tx_exists(tx_hash, tx_index))
    {
        return false;
    }

    return true;
}


bool
CurrentBlockchainStatus::tx_exist(const string& tx_hash_str, uint64_t& tx_index)
{
    crypto::hash tx_hash;

    if (hex_to_pod(tx_hash_str, tx_hash))
    {
        return tx_exist(tx_hash, tx_index);
    }

    return false;
}

bool
CurrentBlockchainStatus::tx_exist(const string& tx_hash_str)
{
    crypto::hash tx_hash;

    if (hex_to_pod(tx_hash_str, tx_hash))
    {
        return tx_exist(tx_hash);
    }

    throw runtime_error("(hex_to_pod(tx_hash_str, tx_hash) failed!");
}

bool
CurrentBlockchainStatus::get_tx_with_output(
        uint64_t output_idx, uint64_t amount,
        transaction& tx, uint64_t& output_idx_in_tx)
{

    tx_out_index tx_out_idx;

    try
    {
        // get pair pair<crypto::hash, uint64_t> where first is tx hash
        // and second is local index of the output i in that tx
        tx_out_idx = core_storage->get_db()
                .get_output_tx_and_index(amount, output_idx);
    }
    catch (const OUTPUT_DNE &e)
    {

        string out_msg = fmt::format(
                "Output with amount {:d} and index {:d} does not exist!",
                amount, output_idx
        );

        cerr << out_msg << endl;

        return false;
    }

    output_idx_in_tx = tx_out_idx.second;

    if (!mcore->get_tx(tx_out_idx.first, tx))
    {
        cerr << "Cant get tx: " << tx_out_idx.first << endl;

        return false;
    }

    return true;
}

bool
CurrentBlockchainStatus::get_output_keys(const uint64_t& amount,
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


string
CurrentBlockchainStatus::get_account_integrated_address_as_str(
        crypto::hash8 const& payment_id8)
{
    return cryptonote::get_account_integrated_address_as_str(net_type,
                    import_payment_address.address, payment_id8);
}

string
CurrentBlockchainStatus::get_account_integrated_address_as_str(
        string const& payment_id8_str)
{
    crypto::hash8 payment_id8;

    if (!hex_to_pod(payment_id8_str, payment_id8))
        return string {};

    return get_account_integrated_address_as_str(payment_id8);
}

bool
CurrentBlockchainStatus::get_amount_specific_indices(const crypto::hash& tx_hash,
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

bool
CurrentBlockchainStatus::get_random_outputs(
        const vector<uint64_t>& amounts,
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


bool
CurrentBlockchainStatus::get_output(
        const uint64_t amount,
        const uint64_t global_output_index,
        COMMAND_RPC_GET_OUTPUTS_BIN::outkey& output_info)
{
    rpccalls rpc {deamon_url};

    string error_msg;

    if (!rpc.get_out(amount, global_output_index, output_info))
    {
        cerr << "rpc.get_out" << endl;
        return false;
    }

    return true;
}


bool
CurrentBlockchainStatus::get_dynamic_per_kb_fee_estimate(uint64_t& fee_estimated)
{
    rpccalls rpc {deamon_url};

    string error_msg;

    if (!rpc.get_dynamic_per_kb_fee_estimate(
            FEE_ESTIMATE_GRACE_BLOCKS,
            fee_estimated, error_msg))
    {
        cerr << "rpc.get_dynamic_per_kb_fee_estimate failed" << endl;
        return false;
    }

    return true;
}


bool
CurrentBlockchainStatus::commit_tx(const string& tx_blob, string& error_msg, bool do_not_relay)
{
    rpccalls rpc {deamon_url};

    if (!rpc.commit_tx(tx_blob, error_msg, do_not_relay))
    {
        cerr << "commit_tx failed" << endl;
        return false;
    }

    return true;
}

bool
CurrentBlockchainStatus::read_mempool()
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

        transaction tx;
        crypto::hash tx_hash;
        crypto::hash tx_prefix_hash;

        if (!parse_and_validate_tx_from_blob(
                _tx_info.tx_blob, tx, tx_hash, tx_prefix_hash))
        {
            cerr << "Cant make tx from _tx_info.tx_blob" << endl;
            return false;
        }

        (void) tx_hash;
        (void) tx_prefix_hash;

        mempool_txs.emplace_back(_tx_info.receive_time, tx);

    } // for (size_t i = 0; i < mempool_tx_info.size(); ++i)

    return true;
}

vector<pair<uint64_t, transaction>>
CurrentBlockchainStatus::get_mempool_txs()
{
    std::lock_guard<std::mutex> lck (getting_mempool_txs);
    return mempool_txs;
}

bool
CurrentBlockchainStatus::search_if_payment_made(
        const string& payment_id_str,
        const uint64_t& desired_amount,
        string& tx_hash_with_payment)
{

    vector<pair<uint64_t, transaction>> mempool_transactions = get_mempool_txs();

    uint64_t current_blockchain_height = current_height;

    vector<transaction> txs_to_check;

    for (auto& mtx: mempool_transactions)
    {
        txs_to_check.push_back(mtx.second);
    }

    // apend txs in last to blocks into the txs_to_check vector
    for (uint64_t blk_i = current_blockchain_height - 10;
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

        // combine mempool txs and txs from given number of
        // last blocks
        txs_to_check.insert(txs_to_check.end(), blk_txs.begin(), blk_txs.end());
    }

    for (transaction& tx: txs_to_check)
    {
        if (is_coinbase(tx))
        {
            // not interested in coinbase txs
            continue;
        }

        string tx_payment_id_str = get_payment_id_as_string(tx);

        // we are interested only in txs with encrypted payments id8
        // they have length of 16 characters.

        if (tx_payment_id_str.length() != 16)
        {
            continue;
        }

        // we have some tx with encrypted payment_id8
        // need to decode it using tx public key, and our
        // private view key, before we can comapre it is
        // what we are after.

        crypto::hash8 encrypted_payment_id8;

        if (!hex_to_pod(tx_payment_id_str, encrypted_payment_id8))
        {
            cerr << "failed parsing hex to pod for encrypted_payment_id8" << '\n';
        }

        // decrypt the encrypted_payment_id8

        public_key tx_pub_key = xmreg::get_tx_pub_key_from_received_outs(tx);


        // public transaction key is combined with our viewkey
        // to create, so called, derived key.
        key_derivation derivation;

        if (!generate_key_derivation(tx_pub_key, import_payment_viewkey, derivation))
        {
            cerr << "Cant get derived key for: "  << "\n"
                 << "pub_tx_key: " << tx_pub_key << " and "
                 << "prv_view_key" << import_payment_viewkey << endl;

            return false;
        }

        // decrypt encrypted payment id, as used in integreated addresses
        crypto::hash8 decrypted_payment_id8 = encrypted_payment_id8;

        if (decrypted_payment_id8 != null_hash8)
        {
            if (!mcore->get_device()->decrypt_payment_id(
                    decrypted_payment_id8, tx_pub_key, import_payment_viewkey))
            {
                cerr << "Cant decrypt  decrypted_payment_id8: "
                     << pod_to_hex(decrypted_payment_id8) << "\n";
            }
        }

        string decrypted_tx_payment_id_str = pod_to_hex(decrypted_payment_id8);

        // check if decrypted payment id matches what we have stored
        // in mysql.
        if (payment_id_str != decrypted_tx_payment_id_str)
        {
            // check tx having specific payment id only
            continue;
        }

        // if everything ok with payment id, we proceed with
        // checking if the amount transfered is correct.

        // for each output, in a tx, check if it belongs
        // to the given account of specific address and viewkey


        //          <public_key  , amount  , out idx>
        vector<tuple<txout_to_key, uint64_t, uint64_t>> outputs;

        outputs = get_ouputs_tuple(tx);

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
                              import_payment_address.address.m_spend_public_key,
                              generated_tx_pubkey);

            // check if generated public key matches the current output's key
            bool mine_output = (txout_k.key == generated_tx_pubkey);

            // if mine output has RingCT, i.e., tx version is 2
            // need to decode its amount. otherwise its zero.
            if (mine_output && tx.version == 2)
            {
                // initialize with regular amount
                uint64_t rct_amount = amount;

                // cointbase txs have amounts in plain sight.
                // so use amount from ringct, only for non-coinbase txs
                if (!is_coinbase(tx))
                {
                    bool r;

                    r = decode_ringct(tx.rct_signatures,
                                      tx_pub_key,
                                      import_payment_viewkey,
                                      output_idx_in_tx,
                                      tx.rct_signatures.ecdhInfo[output_idx_in_tx].mask,
                                      rct_amount);

                    if (!r)
                    {
                        cerr << "Cant decode ringCT!" << endl;
                        throw TxSearchException("Cant decode ringCT!");
                    }

                    amount = rct_amount;
                }

            } // if (mine_output && tx.version == 2)



            if (mine_output)
            {
                total_received += amount;
            }
        }

        cout << " - payment id check in tx: "
             << tx_hash_str
             << " found: " << total_received << endl;

        if (total_received >= desired_amount)
        {
            // the payment has been made.
            tx_hash_with_payment = tx_hash_str;
            return true;
        }
    }

    return false;
}


string
CurrentBlockchainStatus::get_payment_id_as_string(const transaction& tx)
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


output_data_t
CurrentBlockchainStatus::get_output_key(uint64_t amount, uint64_t global_amount_index)
{
    return core_storage->get_db().get_output_key(amount, global_amount_index);
}

bool
CurrentBlockchainStatus::start_tx_search_thread(XmrAccount acc)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (search_thread_exist(acc.address))
    {
        // thread for this address exist, dont make new one
        cout << "Thread exists, dont make new one" << endl;
        return true; // this is still OK, so return true.
    }

    try
    {
        // make a tx_search object for the given xmr account
        //searching_threads.emplace(acc.address, new TxSearch(acc)); // does not work on older gcc
                                                                     // such as the one in ubuntu 16.04
        searching_threads[acc.address] = unique_ptr<TxSearch>(new TxSearch(acc));
    }
    catch (const std::exception& e)
    {
        cerr << "Faild created a search thread " << endl;
        return false;
    }

    // start the thread for the created object
    std::thread t1 {&TxSearch::search, searching_threads[acc.address].get()};
    t1.detach();

    return true;
}



bool
CurrentBlockchainStatus::ping_search_thread(const string& address)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (!search_thread_exist(address))
    {
        // thread does not exist
        cout << "thread for " << address << " does not exist" << endl;
        return false;
    }

    searching_threads[address].get()->ping();

    return true;
}

bool
CurrentBlockchainStatus::get_searched_blk_no(const string& address,
                                             uint64_t& searched_blk_no)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (!search_thread_exist(address))
    {
        // thread does not exist
        cout << "thread for " << address << " does not exist" << endl;
        return false;
    }

    searched_blk_no = searching_threads[address].get()->get_searched_blk_no();

    return true;
}

bool
CurrentBlockchainStatus::get_known_outputs_keys(
        string const& address,
        vector<pair<string, uint64_t>>& known_outputs_keys)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (!search_thread_exist(address))
    {
        // thread does not exist
        cout << "thread for " << address << " does not exist" << endl;
        return false;
    }


    known_outputs_keys
            = searching_threads[address].get()->get_known_outputs_keys();

    return true;
}

bool
CurrentBlockchainStatus::search_thread_exist(const string& address)
{
    // no mutex here, as this will be executed
    // from other methods, which do use mutex.
    // so if you put mutex here, you will get into deadlock.
    return searching_threads.count(address) > 0;
}

bool
CurrentBlockchainStatus::get_xmr_address_viewkey(
        const string& address_str,
        address_parse_info& address,
        secret_key& viewkey)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (!search_thread_exist(address_str))
    {
        // thread does not exist
        cout << "thread for " << address_str << " does not exist" << endl;
        return false;
    }

    address = searching_threads[address_str].get()->get_xmr_address_viewkey().first;
    viewkey = searching_threads[address_str].get()->get_xmr_address_viewkey().second;

    return true;
};

bool
CurrentBlockchainStatus::find_txs_in_mempool(
        const string& address_str,
        json& transactions)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (searching_threads.count(address_str) == 0)
    {
        // thread does not exist
        cout << "thread for " << address_str << " does not exist" << endl;
        return false;
    }

    transactions = searching_threads[address_str].get()
            ->find_txs_in_mempool(mempool_txs);

    return true;
};

bool
CurrentBlockchainStatus::find_tx_in_mempool(
        crypto::hash const& tx_hash,
        transaction& tx)
{

    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    for (auto const& mtx: mempool_txs)
    {
        const transaction &m_tx = mtx.second;

        if (get_transaction_hash(m_tx) == tx_hash)
        {
            tx = m_tx;
            return true;
        }
    }

    return false;
}

bool
CurrentBlockchainStatus::find_key_images_in_mempool(std::vector<txin_v> const& vin)
{
    mempool_txs_t mempool_tx_cpy;

    {
        // make local copy of the mempool, so that we dont lock it for
        // long, as this function can take longer to execute
        std::lock_guard<std::mutex> lck (searching_threads_map_mtx);
        mempool_tx_cpy = mempool_txs;
    }

    // perform exhostive search to check if any key image in vin vector
    // is in the mempool. This is used to check if a tx generated by the frontend
    // is using any key images that area already in the mempool.
    for (auto const& kin: vin)
    {
        if(kin.type() != typeid(txin_to_key))
            continue;

        // get tx input key
        const txin_to_key& tx_in_to_key
                = boost::get<cryptonote::txin_to_key>(kin);

        for (auto const& mtx: mempool_tx_cpy)
        {
            const transaction &m_tx = mtx.second;

            vector<txin_to_key> input_key_imgs = xmreg::get_key_images(m_tx);

            for (auto const& mki: input_key_imgs)
            {
                // if a matching key image found in the mempool
                if (mki.k_image == tx_in_to_key.k_image)
                    return true;
            }
        }
    }

    return false;
}


bool
CurrentBlockchainStatus::find_key_images_in_mempool(transaction const& tx)
{
    return find_key_images_in_mempool(tx.vin);
}


bool
CurrentBlockchainStatus::get_tx(crypto::hash const& tx_hash, transaction& tx)
{
    return mcore->get_tx(tx_hash, tx);
}


bool
CurrentBlockchainStatus::get_tx(string const& tx_hash_str, transaction& tx)
{
    crypto::hash tx_hash;

    if (!hex_to_pod(tx_hash_str, tx_hash))
    {
       return false;
    }

    return mcore->get_tx(tx_hash, tx);
}

bool
CurrentBlockchainStatus::get_tx_block_height(crypto::hash const& tx_hash, int64_t& tx_height)
{
    if (!tx_exist(tx_hash))
        return false;

    tx_height = core_storage->get_db().get_tx_block_height(tx_hash);

    return true;
}

bool
CurrentBlockchainStatus::set_new_searched_blk_no(const string& address, uint64_t new_value)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (searching_threads.count(address) == 0)
    {
        // thread does not exist
        cout << " thread does not exist" << endl;
        return false;
    }

    searching_threads[address].get()->set_searched_blk_no(new_value);

    return true;
}


void
CurrentBlockchainStatus::clean_search_thread_map()
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    for (const auto& st: searching_threads)
    {
        if (search_thread_exist(st.first) && st.second->still_searching() == false)
        {
            cout << st.first << " still searching: " << st.second->still_searching() << endl;
            searching_threads.erase(st.first);
        }
    }
}


tuple<string, string, string>
CurrentBlockchainStatus::construct_output_rct_field(
        const uint64_t global_amount_index,
        const uint64_t out_amount)
{

   transaction random_output_tx;
    uint64_t output_idx_in_tx;

    // we got random outputs, but now we need to get rct data of those
    // outputs, because by default frontend created ringct txs.

    if (!CurrentBlockchainStatus::get_tx_with_output(
            global_amount_index, out_amount,
            random_output_tx, output_idx_in_tx))
    {
        cerr << "cant get random output transaction" << endl;
        return make_tuple(string {}, string {}, string {});
    }

    //cout << pod_to_hex(out.out_key) << endl;
    //cout << pod_to_hex(get_transaction_hash(random_output_tx)) << endl;
    //cout << output_idx_in_tx << endl;

    // placeholder variable for ringct outputs info
    // that we need to save in database
    string rtc_outpk;
    string rtc_mask(64, '0');
    string rtc_amount(64, '0');


    if (random_output_tx.version > 1 && !is_coinbase(random_output_tx))
    {
        rtc_outpk  = pod_to_hex(random_output_tx.rct_signatures.outPk[output_idx_in_tx].mask);
        rtc_mask   = pod_to_hex(random_output_tx.rct_signatures.ecdhInfo[output_idx_in_tx].mask);
        rtc_amount = pod_to_hex(random_output_tx.rct_signatures.ecdhInfo[output_idx_in_tx].amount);
    }
    else
    {
        // for non ringct txs, we need to take it rct amount commitment
        // and sent to the frontend. the mask is zero mask for those,
        // as frontend will produce identy mask autmatically for non-ringct outputs

        output_data_t od = get_output_key(out_amount, global_amount_index);

        rtc_outpk  = pod_to_hex(od.commitment);

        if (is_coinbase(random_output_tx)) // commenting this out. think its not needed.
        {                                  // as this function provides keys for mixin outputs
                                           // not the ones we actually spend.
            // ringct coinbase txs are special. they have identity mask.
            // as suggested by this code:
            // https://github.com/monero-project/monero/blob/eacf2124b6822d088199179b18d4587404408e0f/src/wallet/wallet2.cpp#L893
            // https://github.com/monero-project/monero/blob/master/src/blockchain_db/blockchain_db.cpp#L100
            // rtc_mask   = pod_to_hex(rct::identity());
        }

    }

    return make_tuple(rtc_outpk, rtc_mask, rtc_amount);
};


}