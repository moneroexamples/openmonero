//
// Created by mwo on 15/06/18.
//

#include "src/MicroCore.h"
#include "../src/BlockchainSetup.h"


#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "../src/ThreadRAII.h"


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

class DifferentNetworks :
        public ::testing::TestWithParam<network_type>
{

};

class BlockchainSetupTest : public DifferentNetworks
{};

TEST_P(BlockchainSetupTest, DefaultConstruction)
{
    xmreg::BlockchainSetup bc_setup;
    EXPECT_TRUE(true);
}

TEST_P(BlockchainSetupTest, ReadInConfigFile)
{
    network_type net_type = GetParam();
    string net_name = xmreg::BlockchainSetup::get_network_name(net_type);

    bool do_not_relay {false};
    string confing_path {"../config/config.json"};

    xmreg::BlockchainSetup bc_setup {net_type, do_not_relay, confing_path};

    json config_json = xmreg::BlockchainSetup::read_config(confing_path);

    EXPECT_EQ(bc_setup.import_payment_address_str,
              config_json["wallet_import"][net_name]["address"]);

    // we expact that bc_setup will have json config
    // equal to what we read manually
    EXPECT_EQ(bc_setup.get_config(), config_json);

    xmreg::BlockchainSetup bc_setup2 {net_type, do_not_relay, config_json};

    EXPECT_EQ(bc_setup2.import_payment_address_str,
              config_json["wallet_import"][net_name]["address"]);

}


TEST_P(BlockchainSetupTest, ReadInConfigFileFailure)
{
    network_type net_type = GetParam();

    string net_name = xmreg::BlockchainSetup::get_network_name(net_type);

    bool do_not_relay {false};

    string confing_path {"../non_exisiting/config.json"};

    try
    {
        // expect throw if confing file does not exist
        xmreg::BlockchainSetup bc_setup {net_type, do_not_relay, confing_path};
    }
    catch (std::exception const& e)
    {
        EXPECT_THAT(e.what(), HasSubstr("does not exist"));
    }

    confing_path = "./tests/res/config_ill_formed.json";

    if (!boost::filesystem::exists(confing_path))
        confing_path = "./res/config_ill_formed.json";

    FilePath fp {confing_path};

    ASSERT_TRUE(fp.FileOrDirectoryExists());

    try
    {
        // expect throw if confing file is illformed and cant be parsed
        // into json
        xmreg::BlockchainSetup bc_setup {net_type, do_not_relay, confing_path};
    }
    catch (std::exception const& e)
    {
        EXPECT_THAT(e.what(), HasSubstr("Cant parse json "));
    }
}

TEST_P(BlockchainSetupTest, ParsingConfigFileFailure)
{
    network_type net_type = GetParam();

    string net_name = xmreg::BlockchainSetup::get_network_name(net_type);

    bool do_not_relay {false};

    string config_path {"../config/config.json"};

    json config_json = xmreg::BlockchainSetup::read_config(config_path);

    json wrong_data = config_json;

    // make mistake in address
    wrong_data["wallet_import"][net_name]["address"] = string {"00011231"};

    try
    {
        // expect throw if cant parase address or viewkey
        xmreg::BlockchainSetup bc_setup {net_type, do_not_relay, wrong_data};
    }
    catch (std::exception const& e)
    {
        EXPECT_TRUE(true);
    }

    // provide empty address
    wrong_data = config_json;
    wrong_data["wallet_import"][net_name]["address"] = string {};

    try
    {
        // expect throw if cant parase address or viewkey
        xmreg::BlockchainSetup bc_setup {net_type, do_not_relay, wrong_data};
    }
    catch (std::exception const& e)
    {
        EXPECT_TRUE(true);
    }

    // provide wrong viewkey
    wrong_data = config_json;
    wrong_data["wallet_import"][net_name]["viewkey"] = string {"00011231"};

    try
    {
        // expect throw if cant parase address or viewkey
        xmreg::BlockchainSetup bc_setup {net_type, do_not_relay, wrong_data};
    }
    catch (std::exception const& e)
    {
        EXPECT_TRUE(true);
    }
}

TEST(BlockchainSetupTest, WrongNetworkType)
{
    network_type net_type = network_type::UNDEFINED;

    try
    {
        // expect throw if cant parase address or viewkey
        xmreg::BlockchainSetup::get_network_name(net_type);
    }
    catch (std::exception const& e)
    {
        EXPECT_TRUE(true);
    }

    bool do_not_relay {false};

    string config_path {"../config/config.json"};

    json config_json = xmreg::BlockchainSetup::read_config(config_path);

    try
    {
        // expect throw if cant parase address or viewkey
        xmreg::BlockchainSetup bc_setup {net_type, do_not_relay, config_json};
    }
    catch (std::exception const& e)
    {
        EXPECT_TRUE(true);
    }
}


TEST_P(BlockchainSetupTest, WrongBlockchainPath)
{
    network_type net_type = GetParam();

    string net_name = xmreg::BlockchainSetup::get_network_name(net_type);

    bool do_not_relay {false};

    string config_path {"../config/config.json"};

    json config_json = xmreg::BlockchainSetup::read_config(config_path);

    // make mistake in address
    config_json["blockchain-path"][net_name]
            = string {"/wrong/path/to/bloclckahin"};

    try
    {
        // expect throw if cant parase address or viewkey
        xmreg::BlockchainSetup bc_setup {net_type, do_not_relay, config_json};
    }
    catch (std::exception const& e)
    {
        EXPECT_TRUE(true);
    }
}

INSTANTIATE_TEST_CASE_P(
        DifferentMoneroNetworks, BlockchainSetupTest,
        ::testing::Values(
                network_type::MAINNET,
                network_type::TESTNET,
                network_type::STAGENET));


class MICROCORE_TEST : public ::testing::Test
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

json MICROCORE_TEST::config_json;

TEST_F(MICROCORE_TEST, DefaultConstruction)
{
    xmreg::MicroCore mcore;
    EXPECT_TRUE(true);
}

TEST_F(MICROCORE_TEST, InitializationSuccess)
{
    xmreg::MicroCore mcore;

    EXPECT_TRUE(mcore.init(bc_setup.blockchain_path, net_type));

    EXPECT_TRUE(mcore.get_core().get_db().is_open());
    EXPECT_TRUE(mcore.get_core().get_db().is_read_only());
}

//template <bool open_success>
//class MockBlockchainDB : public BlockchainLMDB
//{
//   void
//   open(const std::string& filename, const int mdb_flags=0) override
//   {
//       if (open_success == false)
//            throw std::runtime_error("Cant open database");
//   }

//   bool is_open() const
//   {
//       return false;
//   }
//};



}
