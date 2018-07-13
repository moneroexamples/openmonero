//
// Created by mwo on 15/06/18.
//

#include "../src/MicroCore.h"
#include "../src/YourMoneroRequests.h"
#include "../src/MysqlPing.h"


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


json
readin_config()
{
    // read in confing json file and get test db info

    std::string config_json_path{"../config/config.json"};

    // check if config-file provided exist
    if (!boost::filesystem::exists(config_json_path))
    {
        std::cerr << "Config file " << config_json_path
                  << " does not exist\n";

        return {};
    }

    json config_json;

    try
    {
        // try reading and parsing json config file provided
        std::ifstream i(config_json_path);
        i >> config_json;

        return config_json;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error reading confing file "
                  << config_json_path << ": "
                  << e.what() << '\n';
        return {};
    }

    return {};
}


class MICROCORE_TEST : public ::testing::Test
{
public:

    static void
    SetUpTestCase()
    {

        db_config = readin_config();

        if (db_config.empty())
            FAIL() << "Cant readin_config()";
    }

protected:


    virtual void
    SetUp()
    {

    }

    static std::string db_data;
    static json db_config;
    std::shared_ptr<xmreg::MySqlAccounts> xmr_accounts;
    std::shared_ptr<xmreg::CurrentBlockchainStatus> current_bc_status;
};

std::string MYSQL_TEST::db_data;
json MYSQL_TEST::db_config;

