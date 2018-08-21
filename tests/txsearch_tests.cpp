
#include "../src/MicroCore.h"
#include "../src/CurrentBlockchainStatus.h"
#include "../src/ThreadRAII.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "helpers.h"
#include "mocks.h"

namespace
{

using json = nlohmann::json;



class OUTPUTIDENT_TEST : public ::testing::Test
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
        bc_setup = xmreg::BlockchainSetup{net_type, do_not_relay, config_json};
        cbs_mock = make_shared<MockCurrentBlockchainStatus>();
    }

     network_type net_type {network_type::TESTNET};
     bool do_not_relay {false};
     xmreg::BlockchainSetup bc_setup;

     std::shared_ptr<xmreg::CurrentBlockchainStatus> cbs_mock;

     static json config_json;
};

json OUTPUTIDENT_TEST::config_json;


TEST_F(OUTPUTIDENT_TEST, DefaultConstruction)
{
    xmreg::OutputInputIdentification oi;

    EXPECT_TRUE(true);
}


string addr_9wq79 {"9wq792k9sxVZiLn66S3Qzv8QfmtcwkdXgM5cWGsXAPxoQeMQ79md51PLPCijvzk1iHbuHi91pws5B7iajTX9KTtJ4bh2tCh"};
string viewkey_9wq79 {"f747f4a4838027c9af80e6364a941b60c538e67e9ea198b6ec452b74c276de06"};

TEST_F(OUTPUTIDENT_TEST, NonDefaultConstruction)
{
    ADDR_VIEWKEY_FROM_STRING(addr_9wq79, viewkey_9wq79, net_type);

    transaction tx;
    crypto::hash tx_hash;
    bool is_coinbase {false};

    xmreg::OutputInputIdentification oi {&address, &viewkey, &tx,
                                         tx_hash, is_coinbase,
                                         cbs_mock};

    EXPECT_TRUE(true);
}

TEST_F(OUTPUTIDENT_TEST, PreRingctTransaction)
{
    // private testnet tx_hash 01e3d29a5b6b6e1999b9e7e48b9abe101bb93055b701b23d98513fb0646e7570
    string tx_hex {"0100020280e08d84ddcb0107041e6508060e1df9733757fb1563579b3a0ecefb4063dc66a75ad6bae3d0916ab862789c31f6aa0280e08d84ddcb01075f030f17031713f43315e59fbd6bacb3aa1a868edc33a0232f5031f6c573d6d7b23e752b6beee10580d0b8e1981a02e04e4d7d674381d16bf50bdc1581f75f7b2df591b7a40d73efd8f5b3c4be2a828088aca3cf02025c3fb06591f216cab833bb6deb2e173e0a9a138addf527eaff80912b8f4ff47280f882ad1602779945c0e6cbbc7def201e170a38bb9822109ddc01f330a5b00d12eb26b6fd4880e0bcefa757024f05eb53b5ebd60e502abd5a2fcb74b057b523acccd191df8076e27083ef619180c0caf384a302022dffb5574d937c1add2cf88bd374af40d577c7cc1a22c3c8805afd49840a60962101945fd9057dd42ef52164368eab1a5f430268e029f83815897d4f1e08b0e00873dbf5b4127be86ba01f23f021a40f29e29110f780fa0be36041ecf00b6c7b480edb090ba58dea640ca19aa4921a56c4b6dcbf11897c53db427dfdbc2f3072480b6c02f409b45c5d2401a19d8cfb13d9124d858d2774d9945f448f19e1e243e004588368813689c1c1be4e1242c633b7a2eb4936546fda57c99dac7fab81e40d0512e39d04ce285f96ac80f6d91ee39157189d73e198fa6b397bd34d8685dbf20e3f869043b686c291f8d4401f673978c683841ec39c45ce06564ccf87c68a080ad17bcce3d7240cd8d57ecbb190fef27578679cdd39ea3c868ab65d6d1d2c20062e921ceea054f755ceef8cd24e54078f9a5bedea2ca59d90ad277bd250c90605b3dd832aa15a4eb01080210ade74638f1558e203c644aa608147c9f596ce3c03023e99f9ca5b3cae41cbd230bc20f2b87f1e06967c852115abc7e56566ddaf09b5774568375fa0f27b45d946cfb2859f1c7a3ad6b7a8e097e955a6ee77c6db0b083dbda85b317dcd77e4be29079420bf683a91ac94feb0f788d5e3dfe72bef028768d76f9ebffd4cb2fd4a314e293df51cb43f12091632e93f4f97fdab7ab60dd50611233fbb1048dccd6478184b914710c894d8116620fcfd09d73ef304c90af210a4724c9e8adb2c47396a67944d6fe827a9b06f7a3c3b6cd2946f328cc306e0a0d194443734cc90fb94ccdb74f8fa7a19690793ddc2b0b06e52d0d4d8530ac227d58a2936fbbf18bbbc2af03443a44ff2a844be527371fedc03c50cce200e8e2b4fdb501e2fba103aafc2487be7faaa83f3894bdcfad873a6697ad930500bc56e28139ef4c6d9b8ee06390b4bcb1b6bfcc6e3136be89e3bdccff50d104906d354569aedfd8b2a5cb62b8738218760a9ebbc5dff3de038ab2e0369f28e3d0d921d28b388acdf69988b5c77120de5317be614da7c774f1f815a7137625da90f0342ca5df7bbc8515066c3d8fa37f1d69727f69e540ff66578bd0e6adf73fa074ce25809e47f06edc9d8ac9f49b4f02b8fd48ef8b03d7a6e823c6e2fc105ee0384a5a3a4bfefc41cf7240847e50121233de0083bbd904903b9879ecdd5a3b701a2196e13e438cf3980ab0b85c5e4e3595c46f034cb393b1e291e3e288678c90e9aac0abe0723520d47e94584ff65dfec8d4d1b1d2c378f87347f429a2178b10ad530bfe406441d7b21c1f0ea04920c9715434b16e6f5c561eab4e8b31040a30b280fc0e3ebc71d1d85a6711591487a50e4ca1362aae564c6e332b97da65c0c07"};

    // the tx has only one output for 10 xmr, so we check
    // if we can identify it.

    TX_FROM_HEX(tx_hex);
    ADDR_VIEWKEY_FROM_STRING(addr_9wq79, viewkey_9wq79, net_type);

    xmreg::OutputInputIdentification oi {&address, &viewkey, &tx,
                                         tx_hash, is_tx_coinbase,
                                         cbs_mock};
    oi.identify_outputs();

    EXPECT_EQ(oi.identified_outputs.size(), 1);
    EXPECT_EQ(oi.identified_inputs.size(), 0);
    EXPECT_EQ(oi.get_tx_hash_str(), tx_hash_str);
    EXPECT_EQ(oi.get_tx_prefix_hash_str(), tx_prefix_hash_str);
    EXPECT_EQ(oi.get_tx_pub_key_str(), tx_pub_key_str);
    EXPECT_EQ(oi.total_received, 10000000000000ull);
    EXPECT_FALSE(oi.tx_is_coinbase);
    EXPECT_FALSE(oi.is_rct);

    auto out_info = oi.identified_outputs[0];

    EXPECT_EQ(pod_to_hex(out_info.pub_key),
              "2dffb5574d937c1add2cf88bd374af40d577c7cc1a22c3c8805afd49840a6096");

    EXPECT_EQ(out_info.amount, 10000000000000ull);
    EXPECT_EQ(out_info.idx_in_tx, 4);
    EXPECT_TRUE(out_info.rtc_outpk.empty());
    EXPECT_TRUE(out_info.rtc_mask.empty());
    EXPECT_TRUE(out_info.rtc_amount.empty());

}

class TXSEARCH_TEST : public ::testing::TestWithParam<network_type>
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
        net_type = GetParam();

        bc_setup = xmreg::BlockchainSetup {
                net_type, do_not_relay, config_json};

        mcore = std::make_unique<MockMicroCore>();
        mcore_ptr = mcore.get();

        rpc = std::make_unique<MockRPCCalls>("dummy deamon url");
        rpc_ptr = rpc.get();

        bcs = std::make_unique<xmreg::CurrentBlockchainStatus>(
                    bc_setup, std::move(mcore), std::move(rpc));
    }

     network_type net_type {network_type::STAGENET};
     bool do_not_relay {false};
     xmreg::BlockchainSetup bc_setup;
     std::unique_ptr<MockMicroCore> mcore;
     std::unique_ptr<MockRPCCalls> rpc;
     std::unique_ptr<xmreg::CurrentBlockchainStatus> bcs;

     MockMicroCore* mcore_ptr;
     MockRPCCalls* rpc_ptr;

     static json config_json;
};

json TXSEARCH_TEST::config_json;



}
