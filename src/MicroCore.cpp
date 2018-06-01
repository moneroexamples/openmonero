//
// Created by mwo on 5/11/15.
//

#include "MicroCore.h"


namespace xmreg
{
/**
 * The constructor is interesting, as
 * m_mempool and m_blockchain_storage depend
 * on each other.
 *
 * So basically m_mempool is initialized with
 * reference to Blockchain (i.e., Blockchain&)
 * and m_blockchain_storage is initialized with
 * reference to m_mempool (i.e., tx_memory_pool&)
 *
 * The same is done in cryptonode::core.
 */
MicroCore::MicroCore():
        m_mempool(m_blockchain_storage),
        m_blockchain_storage(m_mempool)
{
    m_device = &hw::get_device("default");
}


/**
 * Initialized the MicroCore object.
 *
 * Create BlockchainLMDB on the heap.
 * Open database files located in blockchain_path.
 * Initialize m_blockchain_storage with the BlockchainLMDB object.
 */
bool
MicroCore::init(const string& _blockchain_path, network_type nt)
{
    int db_flags = 0;

    blockchain_path = _blockchain_path;

    nettype = nt;

    db_flags |= MDB_RDONLY;
    db_flags |= MDB_NOLOCK;

    BlockchainDB* db = nullptr;
    db = new BlockchainLMDB();

    try
    {
        // try opening lmdb database files
        db->open(blockchain_path, db_flags);
    }
    catch (const std::exception& e)
    {
        cerr << "Error opening database: " << e.what();
        return false;
    }

    // check if the blockchain database
    // is successful opened
    if(!db->is_open())
        return false;

    // initialize Blockchain object to manage
    // the database.
    return m_blockchain_storage.init(db, nettype);
}

/**
* Get m_blockchain_storage.
* Initialize m_blockchain_storage with the BlockchainLMDB object.
*/
Blockchain&
MicroCore::get_core()
{
    return m_blockchain_storage;
}

/**
 * Get block by its height
 *
 * returns true if success
 */
bool
MicroCore::get_block_by_height(const uint64_t& height, block& blk)
{

    crypto::hash block_id;

    try
    {
        block_id = m_blockchain_storage.get_block_id_by_height(height);
    }
    catch (const exception& e)
    {
        cerr << e.what() << endl;
        return false;
    }


    if (!m_blockchain_storage.get_block_by_hash(block_id, blk))
    {
        cerr << "Block with hash " << block_id
             << "and height " << height << " not found!"
             << endl;
        return false;
    }

    return true;
}


bool
MicroCore::get_block_from_height(const uint64_t& height, block& blk)
{

    crypto::hash block_id;

    try
    {
        blk = m_blockchain_storage.get_db().get_block_from_height(height);
    }
    catch (const exception& e)
    {
        cerr << e.what() << endl;
        return false;
    }

    return true;
}



/**
 * Get transaction tx from the blockchain using it hash
 */
bool
MicroCore::get_tx(const crypto::hash& tx_hash, transaction& tx)
{
    if (m_blockchain_storage.have_tx(tx_hash))
    {
        // get transaction with given hash
        tx = m_blockchain_storage.get_db().get_tx(tx_hash);
    }
    else
    {
        cerr << "MicroCore::get_tx tx does not exist in blockchain: " << tx_hash << endl;
        return false;
    }


    return true;
}

bool
MicroCore::get_tx(const string& tx_hash_str, transaction& tx)
{

    // parse tx hash string to hash object
    crypto::hash tx_hash;

    if (!xmreg::parse_str_secret_key(tx_hash_str, tx_hash))
    {
        cerr << "Cant parse tx hash: " << tx_hash_str << endl;
        return false;
    }


    if (!get_tx(tx_hash, tx))
    {
        return false;
    }


    return true;
}




/**
 * Find output with given public key in a given transaction
 */
bool
MicroCore::find_output_in_tx(const transaction& tx,
                             const public_key& output_pubkey,
                             tx_out& out,
                             size_t& output_index)
{

    size_t idx {0};


    // search in the ouputs for an output which
    // public key matches to what we want
    auto it = std::find_if(tx.vout.begin(), tx.vout.end(),
                           [&](const tx_out& o)
                           {
                               const txout_to_key& tx_in_to_key
                                       = boost::get<txout_to_key>(o.target);

                               ++idx;

                               return tx_in_to_key.key == output_pubkey;
                           });

    if (it != tx.vout.end())
    {
        // we found the desired public key
        out = *it;
        output_index = idx > 0 ? idx - 1 : idx;

        //cout << idx << " " << output_index << endl;

        return true;
    }

    return false;
}


/**
 * Returns tx hash in a given block which
 * contains given output's public key
 */
bool
MicroCore::get_tx_hash_from_output_pubkey(const public_key& output_pubkey,
                                          const uint64_t& block_height,
                                          crypto::hash& tx_hash,
                                          cryptonote::transaction& tx_found)
{

    tx_hash = null_hash;

    // get block of given height
    block blk;
    if (!get_block_by_height(block_height, blk))
    {
        cerr << "Cant get block of height: " << block_height << endl;
        return false;
    }


    // get all transactions in the block found
    // initialize the first list with transaction for solving
    // the block i.e. coinbase.
    list<transaction> txs {blk.miner_tx};
    list<crypto::hash> missed_txs;

    if (!m_blockchain_storage.get_transactions(blk.tx_hashes, txs, missed_txs))
    {
        cerr << "Cant find transcations in block: " << block_height << endl;
        return false;
    }

    if (!missed_txs.empty())
    {
        cerr << "Transactions not found in blk: " << block_height << endl;

        for (const crypto::hash& h : missed_txs)
        {
            cerr << " - tx hash: " << h << endl;
        }

        return false;
    }


    // search outputs in each transactions
    // until output with pubkey of interest is found
    for (const transaction& tx : txs)
    {

        tx_out found_out;

        // we dont need here output_index
        size_t output_index;

        if (find_output_in_tx(tx, output_pubkey, found_out, output_index))
        {
            // we found the desired public key
            tx_hash = get_transaction_hash(tx);
            tx_found = tx;

            return true;
        }

    }

    return false;
}


uint64_t
MicroCore::get_blk_timestamp(uint64_t blk_height)
{
    cryptonote::block blk;

    if (!get_block_by_height(blk_height, blk))
    {
        cerr << "Cant get block by height: " << blk_height << endl;
        return 0;
    }

    return blk.timestamp;
}


/**
 * De-initialized Blockchain.
 *
 * since blockchain is opened as MDB_RDONLY
 * need to manually free memory taken on heap
 * by BlockchainLMDB
 */
MicroCore::~MicroCore()
{
    delete &m_blockchain_storage.get_db();
}


bool
init_blockchain(const string& path,
                MicroCore& mcore,
                Blockchain*& core_storage,
                network_type nt)
{

    // initialize the core using the blockchain path
    if (!mcore.init(path, nt))
    {
        cerr << "Error accessing blockchain." << endl;
        return false;
    }

    // get the high level Blockchain object to interact
    // with the blockchain lmdb database
    core_storage = &(mcore.get_core());

    return true;
}

string
MicroCore::get_blkchain_path()
{
    return blockchain_path;
}



// this is not finished!
bool
MicroCore::get_random_outs_for_amounts(const uint64_t& amount,
                                       const uint64_t& no_of_outputs,
                                       vector<pair<uint64_t, public_key>>& found_outputs)
{

    uint64_t total_number_of_outputs = m_blockchain_storage.get_db().get_num_outputs(amount);


    // ensure we don't include outputs that aren't yet eligible to be used
    // outpouts are sorted by height
    while (total_number_of_outputs > 0)
    {
        const tx_out_index toi = m_blockchain_storage.get_db()
                .get_output_tx_and_index(amount, total_number_of_outputs - 1);

        const uint64_t height = m_blockchain_storage.get_db().get_tx_block_height(toi.first);

        if (height + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE <= m_blockchain_storage.get_db().height())
            break;

        --total_number_of_outputs;
    }


    std::unordered_set<uint64_t> seen_indices;

    // for each amount that we need to get mixins for, get <n> random outputs
    // from BlockchainDB where <n> is req.outs_count (number of mixins).
    for (uint64_t i = 0; i <  no_of_outputs; ++i)
    {

    }

    return false;
}


hw::device* const
MicroCore::get_device() const
{
    return m_device;
}


}
