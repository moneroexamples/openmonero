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


bool
MicroCore::get_block_from_height(const uint64_t& height, block& blk)
{

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



hw::device* const
MicroCore::get_device() const
{
    return m_device;
}


}
