//
// Created by mwo on 15/06/18.
//

#include "../src/MicroCore.h"
#include "../src/CurrentBlockchainStatus.h"
#include "../src/ThreadRAII.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"



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
using ::testing::internal::FilePath;


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
        bc_setup = xmreg::BlockchainSetup{net_type, do_not_relay, config_json};
    }  

     network_type net_type {network_type::STAGENET};
     bool do_not_relay {false};
     xmreg::BlockchainSetup bc_setup;

     static json config_json;
};

json BCSTATUS_TEST::config_json;

TEST_F(BCSTATUS_TEST, DefaultConstruction)
{
    xmreg::CurrentBlockchainStatus bcs {bc_setup, nullptr};
    EXPECT_TRUE(true);
}

class MockMicroCore : public xmreg::MicroCore
{
    bool
    init(const string& _blockchain_path, network_type nt)
    {
        return true;
    }
};

TEST_F(BCSTATUS_TEST, InitMoneroBlockchain)
{
    std::unique_ptr<MockMicroCore> mcore = std::make_unique<MockMicroCore>();
    xmreg::CurrentBlockchainStatus bcs {bc_setup, std::move(mcore)};
    EXPECT_TRUE(bcs.init_monero_blockchain());
}


}
