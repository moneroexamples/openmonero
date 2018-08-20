
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
    }

     network_type net_type {network_type::STAGENET};
     bool do_not_relay {false};
     xmreg::BlockchainSetup bc_setup;

     static json config_json;
};

json OUTPUTIDENT_TEST::config_json;


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
