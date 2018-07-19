//
// Created by mwo on 15/06/18.
//

#include "../src/MicroCore.h"
#include "../src/CurrentBlockchainStatus.h"
#include "../src/ThreadRAII.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "helpers.h"

namespace
{


using json = nlohmann::json;
using namespace std;
using namespace cryptonote;
using namespace epee::string_tools;
using namespace std::chrono_literals;

using ::testing::AllOf;
using ::testing::Ge;
using ::testing::Le;
using ::testing::HasSubstr;
using ::testing::Not;
using ::testing::Return;
using ::testing::Throw;
using ::testing::DoAll;
using ::testing::SetArgReferee;
using ::testing::_;
using ::testing::internal::FilePath;


class MockMicroCore : public xmreg::MicroCore
{
public:    
    MOCK_METHOD2(init, bool(const string& _blockchain_path, network_type nt));
    MOCK_CONST_METHOD0(get_current_blockchain_height, uint64_t());
    MOCK_CONST_METHOD2(get_block_from_height, bool(uint64_t height, block& blk));
    MOCK_CONST_METHOD2(get_blocks_range, std::vector<block>(const uint64_t& h1, const uint64_t& h2));
    MOCK_CONST_METHOD3(get_transactions, bool(const std::vector<crypto::hash>& txs_ids,
                                              std::vector<transaction>& txs,
                                              std::vector<crypto::hash>& missed_txs));
    MOCK_CONST_METHOD1(have_tx, bool(crypto::hash const& tx_hash));
    MOCK_CONST_METHOD2(tx_exists, bool(crypto::hash const& tx_hash, uint64_t& tx_id));
    MOCK_CONST_METHOD2(get_output_tx_and_index, tx_out_index(uint64_t const& amount, uint64_t const& index));
    MOCK_CONST_METHOD2(get_tx, bool(crypto::hash const& tx_hash, transaction& tx));

};


class BCSTATUS_TEST : public ::testing::Test
{
public:

    static void
    SetUpTestCase()
    {
        string config_path {"../config/config.json"};
        config_json = xmreg::BlockchainSetup::read_config(config_path);        
    }

protected:

    virtual void
    SetUp()
    {        
        bc_setup = xmreg::BlockchainSetup {
                net_type, do_not_relay, config_json};
        mcore = std::make_unique<MockMicroCore>();
        mcore_ptr = mcore.get();
        bcs = std::make_unique<xmreg::CurrentBlockchainStatus>(
                    bc_setup, std::move(mcore));
    }  

     network_type net_type {network_type::STAGENET};
     bool do_not_relay {false};
     xmreg::BlockchainSetup bc_setup;
     std::unique_ptr<MockMicroCore> mcore;
     std::unique_ptr<xmreg::CurrentBlockchainStatus> bcs;

     MockMicroCore* mcore_ptr;

     static json config_json;
};

json BCSTATUS_TEST::config_json;

TEST_F(BCSTATUS_TEST, DefaultConstruction)
{
    xmreg::CurrentBlockchainStatus bcs {bc_setup, nullptr};
    EXPECT_TRUE(true);
}



TEST_F(BCSTATUS_TEST, InitMoneroBlockchain)
{
    EXPECT_CALL(*mcore_ptr, init(_, _))
            .WillOnce(Return(true));

    EXPECT_TRUE(bcs->init_monero_blockchain());
}

TEST_F(BCSTATUS_TEST, GetBlock)
{
   EXPECT_CALL(*mcore_ptr, get_block_from_height(_, _))
           .WillOnce(Return(true));

    uint64_t height = 1000;
    block blk;

    EXPECT_TRUE(bcs->get_block(height, blk));
}


ACTION(ThrowBlockDNE)
{
    throw BLOCK_DNE("Mock Throw: Block does not exist!");
}

TEST_F(BCSTATUS_TEST, GetBlockRange)
{

   vector<block> blocks_to_return {block(), block(), block()};

   EXPECT_CALL(*mcore_ptr, get_blocks_range(_, _))
           .WillOnce(Return(blocks_to_return));

    uint64_t h1 = 1000;
    uint64_t h2 = h1+2;

    vector<block> blocks = bcs->get_blocks_range(h1, h2);

    EXPECT_EQ(blocks, blocks_to_return);

    EXPECT_CALL(*mcore_ptr, get_blocks_range(_, _))
            .WillOnce(ThrowBlockDNE());

    blocks = bcs->get_blocks_range(h1, h2);

    EXPECT_TRUE(blocks.empty());
}

TEST_F(BCSTATUS_TEST, GetBlockTxs)
{
    EXPECT_CALL(*mcore_ptr, get_transactions(_, _, _))
            .WillOnce(Return(true));

    const block dummy_blk;
    vector<transaction> blk_txs;
    vector<crypto::hash> missed_txs;

    EXPECT_TRUE(bcs->get_block_txs(dummy_blk, blk_txs, missed_txs));

    EXPECT_CALL(*mcore_ptr, get_transactions(_, _, _))
            .WillOnce(Return(false));

    EXPECT_FALSE(bcs->get_block_txs(dummy_blk, blk_txs, missed_txs));
}

TEST_F(BCSTATUS_TEST, GetTxs)
{
    EXPECT_CALL(*mcore_ptr, get_transactions(_, _, _))
            .WillOnce(Return(true));

    vector<crypto::hash> txs_to_get;
    vector<transaction> blk_txs;
    vector<crypto::hash> missed_txs;

    EXPECT_TRUE(bcs->get_txs(txs_to_get, blk_txs, missed_txs));

    EXPECT_CALL(*mcore_ptr, get_transactions(_, _, _))
            .WillOnce(Return(false));

    EXPECT_FALSE(bcs->get_txs(txs_to_get, blk_txs, missed_txs));
}

TEST_F(BCSTATUS_TEST, TxExist)
{
    EXPECT_CALL(*mcore_ptr, have_tx(_))
            .WillOnce(Return(true));

    EXPECT_TRUE(bcs->tx_exist(crypto::hash()));

    uint64_t mock_tx_index_to_return {4444};

    // return true and set tx_index (ret by ref) to mock_tx_index_to_return
    EXPECT_CALL(*mcore_ptr, tx_exists(_, _))
            .WillOnce(DoAll(SetArgReferee<1>(mock_tx_index_to_return),
                            Return(true)));

    uint64_t tx_index {0};

    EXPECT_TRUE(bcs->tx_exist(crypto::hash(), tx_index));
    EXPECT_EQ(tx_index, mock_tx_index_to_return);

    // just some dummy hash
    string tx_hash_str
        {"fc4b8d5956b30dc4a353b171b4d974697dfc32730778f138a8e7f16c11907691"};

    tx_index = 0;

    EXPECT_CALL(*mcore_ptr, tx_exists(_, _))
            .WillOnce(DoAll(SetArgReferee<1>(mock_tx_index_to_return),
                            Return(true)));

    EXPECT_TRUE(bcs->tx_exist(tx_hash_str, tx_index));
    EXPECT_EQ(tx_index, mock_tx_index_to_return);

    tx_hash_str = "wrong_hash";

    EXPECT_FALSE(bcs->tx_exist(tx_hash_str, tx_index));
}


TEST_F(BCSTATUS_TEST, GetTxWithOutput)
{
    // some dummy tx hash
    RAND_TX_HASH();

    const tx_out_index tx_idx_to_return = make_pair(tx_hash, 6);

    EXPECT_CALL(*mcore_ptr, get_output_tx_and_index(_, _))
            .WillOnce(Return(tx_idx_to_return));

    EXPECT_CALL(*mcore_ptr, get_tx(_, _))
            .WillOnce(Return(true));

    const uint64_t mock_output_idx {4};
    const uint64_t mock_amount {11110};

    transaction tx_returned;
    uint64_t out_idx_returned;

    EXPECT_TRUE(bcs->get_tx_with_output(mock_output_idx, mock_amount,
                                        tx_returned, out_idx_returned));
}

ACTION(ThrowOutputDNE)
{
    throw OUTPUT_DNE("Mock Throw: Output does not exist!");
}

TEST_F(BCSTATUS_TEST, GetTxWithOutputFailure)
{
    // some dummy tx hash
    RAND_TX_HASH();

    const tx_out_index tx_idx_to_return = make_pair(tx_hash, 6);

    EXPECT_CALL(*mcore_ptr, get_output_tx_and_index(_, _))
            .WillOnce(Return(tx_idx_to_return));

    EXPECT_CALL(*mcore_ptr, get_tx(_, _))
            .WillOnce(Return(false));

    const uint64_t mock_output_idx {4};
    const uint64_t mock_amount {11110};

    transaction tx_returned;
    uint64_t out_idx_returned;

    EXPECT_FALSE(bcs->get_tx_with_output(mock_output_idx, mock_amount,
                                        tx_returned, out_idx_returned));

    // or

    EXPECT_CALL(*mcore_ptr, get_output_tx_and_index(_, _))
            .WillOnce(ThrowOutputDNE());

    EXPECT_FALSE(bcs->get_tx_with_output(mock_output_idx, mock_amount,
                                        tx_returned, out_idx_returned));
}

TEST_F(BCSTATUS_TEST, GetCurrentHeight)
{
    uint64_t mock_current_height {1619148};

    EXPECT_CALL(*mcore_ptr, get_current_blockchain_height())
            .WillOnce(Return(mock_current_height));

    bcs->update_current_blockchain_height();

    EXPECT_EQ(bcs->get_current_blockchain_height(),
              mock_current_height - 1);
}

TEST_F(BCSTATUS_TEST, IsTxSpendtimeUnlockedScenario1)
{
    // there are two main scenerious here.
    // Scenerio 1: tx_unlock_time is block height
    // Scenerio 2: tx_unlock_time is timestamp.

    const uint64_t mock_current_height {100};

    EXPECT_CALL(*mcore_ptr, get_current_blockchain_height())
            .WillOnce(Return(mock_current_height));

    bcs->update_current_blockchain_height();

    // SCENARIO 1: tx_unlock_time is block height

    // expected unlock time is in future, thus a tx is still locked

    uint64_t tx_unlock_time {mock_current_height
                + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE};

    uint64_t not_used_block_height {0}; // not used in the first
                                        // part of the test case

    EXPECT_FALSE(bcs->is_tx_unlocked(
                     tx_unlock_time, not_used_block_height));

    // expected unlock time is in the future
    // (1 blocks from now), thus a tx is locked

    tx_unlock_time = mock_current_height + 1;

    EXPECT_FALSE(bcs->is_tx_unlocked(
                     tx_unlock_time, not_used_block_height));

    // expected unlock time is in the past
    // (10 blocks behind), thus a tx is unlocked

    tx_unlock_time = mock_current_height
            - CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE;

    EXPECT_TRUE(bcs->is_tx_unlocked(tx_unlock_time,
                                              not_used_block_height));

    // expected unlock time is same as as current height
    // thus a tx is unlocked

    tx_unlock_time = mock_current_height;

    EXPECT_TRUE(bcs->is_tx_unlocked(tx_unlock_time,
                                              not_used_block_height));
}


class MockTxUnlockChecker : public xmreg::TxUnlockChecker
{
public:

    // mock system call to get current timestamp
    MOCK_CONST_METHOD0(get_current_time, uint64_t());
    //MOCK_CONST_METHOD1(get_leeway, uint64_t(uint64_t tx_block_height));
};

TEST_F(BCSTATUS_TEST, IsTxSpendtimeUnlockedScenario2)
{
    // there are two main scenerious here.
    // Scenerio 1: tx_unlock_time is block height
    // Scenerio 2: tx_unlock_time is timestamp.

    const uint64_t mock_current_height {100};

    EXPECT_CALL(*mcore_ptr, get_current_blockchain_height())
            .WillOnce(Return(mock_current_height));

    bcs->update_current_blockchain_height();

    // SCENARIO 2: tx_unlock_time is timestamp.

    MockTxUnlockChecker mock_tx_unlock_checker;

    const uint64_t current_timestamp {1000000000};

    EXPECT_CALL(mock_tx_unlock_checker, get_current_time())
            .WillRepeatedly(Return(1000000000));

    uint64_t block_height = mock_current_height;

    // tx unlock time is now
    uint64_t tx_unlock_time {current_timestamp}; // mock timestamp

    EXPECT_TRUE(bcs->is_tx_unlocked(tx_unlock_time, block_height,
                                    mock_tx_unlock_checker));

    // unlock time is 1 second into the future
    tx_unlock_time = current_timestamp
            + mock_tx_unlock_checker.get_leeway(
                block_height, bcs->get_bc_setup().net_type) + 1;

    EXPECT_FALSE(bcs->is_tx_unlocked(tx_unlock_time, block_height,
                                     mock_tx_unlock_checker));

}

}
