//
// Created by mwo on 7/01/17.
//

#include "CurrentBlockchainStatus.h"




#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "openmonero"

namespace xmreg
{

CurrentBlockchainStatus::CurrentBlockchainStatus(
        BlockchainSetup _bc_setup,
        std::unique_ptr<MicroCore> _mcore,
        std::unique_ptr<RPCCalls> _rpc)
    : bc_setup {_bc_setup},
      mcore {std::move(_mcore)},
      rpc {std::move(_rpc)}
{

}

void
CurrentBlockchainStatus::start_monitor_blockchain_thread()
{
    TxSearch::set_search_thread_life(
                bc_setup.search_thread_life_in_seconds);

    if (!is_running)
    {
        m_thread = std::thread{[this]()
           {
               while (true)
               {
                   update_current_blockchain_height();
                   read_mempool();
                   OMINFO << "Current blockchain height: " << current_height
                          << ", no of mempool txs: " << mempool_txs.size();
                   clean_search_thread_map();
                   std::this_thread::sleep_for(
                           std::chrono::seconds(
                            bc_setup
                             .refresh_block_status_every_seconds));
               }
           }};

        is_running = true;
    }
}


uint64_t
CurrentBlockchainStatus::get_current_blockchain_height()
{
    return current_height;
}


void
CurrentBlockchainStatus::update_current_blockchain_height()
{
    current_height = mcore->get_current_blockchain_height() - 1;
}

bool
CurrentBlockchainStatus::init_monero_blockchain()
{
    // initialize the core using the blockchain path
    return mcore->init(bc_setup.blockchain_path, bc_setup.net_type);}

// taken from
// bool wallet2::is_transfer_unlocked(uint64_t unlock_time,
//uint64_t block_height) const
bool
CurrentBlockchainStatus::is_tx_unlocked(
        uint64_t unlock_time,
        uint64_t block_height,
        TxUnlockChecker const& tx_unlock_checker)
{
    return tx_unlock_checker.is_unlocked(bc_setup.net_type,
                                         current_height,
                                         unlock_time,
                                         block_height);
}


bool
CurrentBlockchainStatus::get_block(uint64_t height, block &blk)
{
    return mcore->get_block_from_height(height, blk);
}

vector<block>
CurrentBlockchainStatus::get_blocks_range(
        uint64_t const& h1, uint64_t const& h2)
{
    try
    {
        return mcore->get_blocks_range(h1, h2);
    }
    catch (BLOCK_DNE& e)
    {
        cerr << e.what() << endl;
    }

    return {};
}

bool
CurrentBlockchainStatus::get_block_txs(
        const block &blk,
        vector<transaction>& blk_txs,
        vector<crypto::hash>& missed_txs)
{
    // get all transactions in the block found
    // initialize the first list with transaction for solving
    // the block i.e. coinbase tx.
    blk_txs.push_back(blk.miner_tx);

    if (!mcore->get_transactions(blk.tx_hashes, blk_txs, missed_txs))
    {
        OMERROR << "Cant get transactions in block: "
                << get_block_hash(blk);

        return false;
    }

    return true;
}

bool
CurrentBlockchainStatus::get_txs(
        vector<crypto::hash> const& txs_to_get,
        vector<transaction>& txs,
        vector<crypto::hash>& missed_txs)
{
    if (!mcore->get_transactions(txs_to_get, txs, missed_txs))
    {
        OMERROR << "CurrentBlockchainStatus::get_txs: "
                   "cant get transactions!";
        return false;
    }

    return true;
}

bool
CurrentBlockchainStatus::tx_exist(const crypto::hash& tx_hash)
{
    return mcore->have_tx(tx_hash);
}

bool
CurrentBlockchainStatus::tx_exist(
        const crypto::hash& tx_hash,
        uint64_t& tx_index)
{
    return mcore->tx_exists(tx_hash, tx_index);
}


bool
CurrentBlockchainStatus::tx_exist(
        const string& tx_hash_str,
        uint64_t& tx_index)
{
    crypto::hash tx_hash;

    if (hex_to_pod(tx_hash_str, tx_hash))
    {
        return tx_exist(tx_hash, tx_index);
    }

    return false;
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
        tx_out_idx = mcore->get_output_tx_and_index(amount, output_idx);
    }
    catch (const OUTPUT_DNE &e)
    {

        string out_msg = fmt::format(
                "Output with amount {:d} and index {:d} does not exist!",
                amount, output_idx
        );

        OMERROR << out_msg;

        return false;
    }

    output_idx_in_tx = tx_out_idx.second;

    if (!mcore->get_tx(tx_out_idx.first, tx))
    {
        OMERROR << "Cant get tx: " << tx_out_idx.first;

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
        mcore->get_output_key(amount, absolute_offsets, outputs);
        return true;
    }
    catch (const OUTPUT_DNE& e)
    {
        OMERROR << "get_output_keys: " << e.what();
    }

    return false;
}


string
CurrentBlockchainStatus::get_account_integrated_address_as_str(
        crypto::hash8 const& payment_id8)
{
    return cryptonote::get_account_integrated_address_as_str(
            bc_setup.net_type,
            bc_setup.import_payment_address.address, payment_id8);
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
CurrentBlockchainStatus::get_amount_specific_indices(
        const crypto::hash& tx_hash,
        vector<uint64_t>& out_indices)
{
    try
    {
        // this index is lmdb index of a tx, not tx hash
        uint64_t tx_index;

        if (mcore->tx_exists(tx_hash, tx_index))
        {
            out_indices = mcore->get_tx_amount_output_indices(tx_index);

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
        vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS
            ::outs_for_amount>& found_outputs)
{

    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request req;
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response res;

    req.outs_count = outs_count;
    req.amounts = amounts;

    if (!mcore->get_random_outs_for_amounts(req, res))
    {
        OMERROR << "mcore->get_random_outs_for_amounts(req, res) failed";
        return false;
    }

    found_outputs = res.outs;

    return true;
}

bool
CurrentBlockchainStatus::get_output(
        const uint64_t amount,
        const uint64_t global_output_index,
        COMMAND_RPC_GET_OUTPUTS_BIN::outkey& output_info)
{
    COMMAND_RPC_GET_OUTPUTS_BIN::request req;
    COMMAND_RPC_GET_OUTPUTS_BIN::response res;

    req.outputs.push_back(get_outputs_out {amount, global_output_index});

    if (!mcore->get_outs(req, res))
    {
        OMERROR << "mcore->get_outs(req, res) failed";
        return false;
    }

    output_info = res.outs.at(0);

    return true;
}

uint64_t
CurrentBlockchainStatus::get_dynamic_per_kb_fee_estimate() const
{
    return mcore->get_dynamic_per_kb_fee_estimate(
                FEE_ESTIMATE_GRACE_BLOCKS);
}


bool
CurrentBlockchainStatus::commit_tx(
        const string& tx_blob,
        string& error_msg,
        bool do_not_relay)
{   

    if (!rpc->commit_tx(tx_blob, error_msg, do_not_relay))
    {
        OMERROR << "rpc->commit_tx() commit_tx failed";
        return false;
    }

    return true;
}

bool
CurrentBlockchainStatus::read_mempool()
{
    // get txs in the mempool
    std::vector<tx_info> mempool_tx_info;
    vector<spent_key_image_info> key_image_infos;

    if (!mcore->get_mempool_txs(mempool_tx_info, key_image_infos))
    {
        OMERROR << "Getting mempool failed ";
        return false;
    }

    // not using this info at present
    (void) key_image_infos;

    std::lock_guard<std::mutex> lck (getting_mempool_txs);

    // clear current mempool txs vector
    // repopulate it with each execution of read_mempool()
    // not very efficient but good enough for now.
    mempool_txs.clear();

    // if dont have tx_blob member, construct tx
    // from json obtained from the rpc call

    for (size_t i = 0; i < mempool_tx_info.size(); ++i)
    {
        // get transaction info of the tx in the mempool
        tx_info const& _tx_info = mempool_tx_info.at(i);

        transaction tx;
        crypto::hash tx_hash;
        crypto::hash tx_prefix_hash;

        if (!parse_and_validate_tx_from_blob(
                _tx_info.tx_blob, tx, tx_hash, tx_prefix_hash))
        {
            OMERROR << "Cant make tx from _tx_info.tx_blob";
            return false;
        }

        (void) tx_hash;
        (void) tx_prefix_hash;

        mempool_txs.emplace_back(_tx_info.receive_time, tx);

    } // for (size_t i = 0; i < mempool_tx_info.size(); ++i)

    return true;
}

CurrentBlockchainStatus::mempool_txs_t
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

    mempool_txs_t mempool_transactions = get_mempool_txs();

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
            OMERROR << "Cant get block of height: " << blk_i;
            return false;
        }

        vector<cryptonote::transaction> blk_txs;
        vector<crypto::hash> missed_txs;

        if (!get_block_txs(blk, blk_txs, missed_txs))
        {
            OMERROR << "Cant get transactions from block: " << blk_i;
            return false;
        }

        // combine mempool txs and txs from given number of
        // last blocks
        txs_to_check.insert(txs_to_check.end(),
                            blk_txs.begin(), blk_txs.end());
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
            OMERROR << "Failed parsing hex to pod for encrypted_payment_id8";
            continue;
        }

        // decrypt the encrypted_payment_id8

        public_key tx_pub_key
                = xmreg::get_tx_pub_key_from_received_outs(tx);


        // public transaction key is combined with our viewkey
        // to create, so called, derived key.
        key_derivation derivation;

        if (!generate_key_derivation(tx_pub_key,
                                     bc_setup.import_payment_viewkey,
                                     derivation))
        {
            OMERROR << "Cant get derived key for: "  << "\n"
                    << "pub_tx_key: " << tx_pub_key << " and "
                    << "prv_view_key" << bc_setup.import_payment_viewkey;

            return false;
        }

        // decrypt encrypted payment id, as used in integreated addresses
        crypto::hash8 decrypted_payment_id8 = encrypted_payment_id8;

        if (decrypted_payment_id8 != null_hash8)
        {
            if (!mcore->decrypt_payment_id(
                    decrypted_payment_id8, tx_pub_key,
                        bc_setup.import_payment_viewkey))
            {
                OMERROR << "Cant decrypt decrypted_payment_id8: "
                        << pod_to_hex(decrypted_payment_id8);
                continue;
            }
        }

        string decrypted_tx_payment_id_str
                = pod_to_hex(decrypted_payment_id8);

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
                              bc_setup.import_payment_address
                                .address.m_spend_public_key,
                              generated_tx_pubkey);

            // check if generated public key matches the current
            // output's key
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
                                      bc_setup.import_payment_viewkey,
                                      output_idx_in_tx,
                                      tx.rct_signatures
                                        .ecdhInfo[output_idx_in_tx].mask,
                                      rct_amount);

                    if (!r)
                    {
                        OMERROR << "Cant decode ringCT!";
                        return false;
                    }

                    amount = rct_amount;
                }

            } // if (mine_output && tx.version == 2)


            if (mine_output)
                total_received += amount;            
        }

        OMINFO << " Payment id check in tx: "
               << tx_hash_str
               << " found: " << total_received;

        if (total_received >= desired_amount)
        {
            // the payment has been made.
            tx_hash_with_payment = tx_hash_str;
            OMINFO << "Import payment done";
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
CurrentBlockchainStatus::get_output_key(
        uint64_t amount,
        uint64_t global_amount_index)
{
    return mcore->get_output_key(amount, global_amount_index);
}

bool
CurrentBlockchainStatus::start_tx_search_thread(XmrAccount acc)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (search_thread_exist(acc.address))
    {
        // thread for this address exist, dont make new one
        //cout << "Thread exists, dont make new one\n";
        return true; // this is still OK, so return true.
    }

    try
    {
        // make a tx_search object for the given xmr account
        //searching_threads.emplace(acc.address, new TxSearch(acc));
        // does not work on older gcc
        // such as the one in ubuntu 16.04
        searching_threads[acc.address]
                = make_unique<TxSearch>(acc, shared_from_this());
    }
    catch (const std::exception& e)
    {
        OMERROR << "Faild created a search thread: " << e.what();
        return false;
    }

    // start the thread for the created object
    std::thread t1 {
        &TxSearch::search,
        searching_threads[acc.address].get()};

    t1.detach();

    OMINFO << "Search thread created for address " << acc.address;

    return true;
}



bool
CurrentBlockchainStatus::ping_search_thread(const string& address)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (!search_thread_exist(address))
    {
        // thread does not exist
        OMERROR << "thread for " << address << " does not exist";
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
        OMERROR << "thread for " << address << " does not exist";
        return false;
    }

    searched_blk_no = searching_threads[address].get()
            ->get_searched_blk_no();

    return true;
}

bool
CurrentBlockchainStatus::get_known_outputs_keys(
        string const& address,
        unordered_map<public_key, uint64_t>& known_outputs_keys)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (!search_thread_exist(address))
    {
        // thread does not exist
        OMERROR << "thread for " << address << " does not exist";
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
        OMERROR << "thread for " << address_str << " does not exist";
        return false;
    }

    address = searching_threads[address_str].get()
            ->get_xmr_address_viewkey().first;
    viewkey = searching_threads[address_str].get()
            ->get_xmr_address_viewkey().second;

    return true;
}

bool
CurrentBlockchainStatus::find_txs_in_mempool(
        const string& address_str,
        json& transactions)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (searching_threads.count(address_str) == 0)
    {
        // thread does not exist
        OMERROR << "thread for " << address_str << " does not exist";
        return false;
    }

    transactions = searching_threads[address_str].get()
            ->find_txs_in_mempool(mempool_txs);

    return true;
}

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
CurrentBlockchainStatus::find_key_images_in_mempool(
        std::vector<txin_v> const& vin)
{
    mempool_txs_t mempool_tx_cpy;

    {
        // make local copy of the mempool, so that we dont lock it for
        // long, as this function can take longer to execute
        std::lock_guard<std::mutex> lck (searching_threads_map_mtx);
        mempool_tx_cpy = mempool_txs;
    }

    // perform exhostive search to check if any key image in vin vector
    // is in the mempool. This is used to check if a tx generated
    // by the frontend
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

            vector<txin_to_key> input_key_imgs
                    = xmreg::get_key_images(m_tx);

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
CurrentBlockchainStatus::find_key_images_in_mempool(
        transaction const& tx)
{
    return find_key_images_in_mempool(tx.vin);
}


bool
CurrentBlockchainStatus::get_tx(
        crypto::hash const& tx_hash,
        transaction& tx)
{
    return mcore->get_tx(tx_hash, tx);
}


bool
CurrentBlockchainStatus::get_tx(
        string const& tx_hash_str,
        transaction& tx)
{
    crypto::hash tx_hash;

    if (!hex_to_pod(tx_hash_str, tx_hash))
    {
       return false;
    }

    return mcore->get_tx(tx_hash, tx);
}

bool
CurrentBlockchainStatus::get_tx_block_height(
        crypto::hash const& tx_hash,
        int64_t& tx_height)
{
    if (!tx_exist(tx_hash))
        return false;

    tx_height = mcore->get_tx_block_height(tx_hash);

    return true;
}

bool
CurrentBlockchainStatus::set_new_searched_blk_no(
        const string& address, uint64_t new_value)
{
    std::lock_guard<std::mutex> lck (searching_threads_map_mtx);

    if (searching_threads.count(address) == 0)
    {
        // thread does not exist
        OMERROR << " thread does not exist";
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
        if (search_thread_exist(st.first)
                && st.second->still_searching() == false)
        {
            OMERROR << st.first << " still searching: "
                 << st.second->still_searching();
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
        OMERROR << "cant get random output transaction";
        return make_tuple(string {}, string {}, string {});
    }

    // placeholder variable for ringct outputs info
    // that we need to save in database
    string rtc_outpk;
    string rtc_mask(64, '0');
    string rtc_amount(64, '0');

    if (random_output_tx.version > 1 && !is_coinbase(random_output_tx))
    {
        rtc_outpk  = pod_to_hex(random_output_tx.rct_signatures
                                .outPk[output_idx_in_tx].mask);
        rtc_mask   = pod_to_hex(random_output_tx.rct_signatures
                                .ecdhInfo[output_idx_in_tx].mask);
        rtc_amount = pod_to_hex(random_output_tx.rct_signatures
                                .ecdhInfo[output_idx_in_tx].amount);
    }
    else
    {
        // for non ringct txs, we need to take it rct amount commitment
        // and sent to the frontend. the mask is zero mask for those,
        // as frontend will produce identy mask autmatically
        //for non-ringct outputs

        output_data_t od = get_output_key(out_amount, global_amount_index);

        rtc_outpk  = pod_to_hex(od.commitment);

        if (is_coinbase(random_output_tx))
        {
            // commenting this out. think its not needed.
            // as this function provides keys for mixin outputs
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



bool
CurrentBlockchainStatus::get_txs_in_blocks(
        vector<block> const& blocks,
        vector<txs_tuple_t>& txs_data)
{
    // get height of the first block
    uint64_t h1 = get_block_height(blocks[0]);

    for(uint64_t blk_i = 0; blk_i < blocks.size(); blk_i++)
    {
        block const& blk = blocks[blk_i];
        uint64_t blk_height = h1 + blk_i;

        // first insert miner_tx
        txs_data.emplace_back(get_transaction_hash(blk.miner_tx),
                              transaction{}, blk_height,
                              blk.timestamp, true);

        // now insert hashes of regular txs to be fatched later
        // so for now, theys txs are null pointers
        for (auto& tx_hash: blk.tx_hashes)
            txs_data.emplace_back(tx_hash, transaction{},
                                  blk_height, blk.timestamp, false);
    }

    // prepare vector<tx_hash> for CurrentBlockchainStatus::get_txs
    // to fetch the correspoding transactions
    std::vector<crypto::hash> txs_to_get;

    for (auto const& tx_tuple: txs_data)
        txs_to_get.push_back(std::get<0>(tx_tuple));

    // fetch all txs from the blocks that we are
    // analyzing in this iteration
    vector<cryptonote::transaction> txs;
    vector<crypto::hash> missed_txs;

    if (!CurrentBlockchainStatus::get_txs(txs_to_get, txs, missed_txs)
            || !missed_txs.empty())
    {
        OMERROR << "Cant get transactions in blocks from : " << h1;
        return false;
    }

    size_t tx_idx {0};

    for (auto& tx: txs)
    {
        auto& tx_tuple = txs_data[tx_idx++];
        //assert(std::get<0>(tx_tuple) == get_transaction_hash(tx));
        std::get<1>(tx_tuple) = std::move(tx);
    }

    return true;
}

}

