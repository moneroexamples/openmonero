//
// Created by mwo on 15/06/18.
//

#include "../src/MicroCore.h"
#include "../src/YourMoneroRequests.h"

#include "gtest/gtest.h"

using json = nlohmann::json;

namespace
{
// set mysql/mariadb connection details
}

class MYSQL_TEST : public ::testing::Test
{
protected:

    static void SetUpTestCase()
    {
        // read in confing json file and get test db info

        std::string config_json_path {"../config/config.json"};

        // check if config-file provided exist
        if (!boost::filesystem::exists(config_json_path))
        {
            std::cerr << "Config file " << config_json_path
                 << " does not exist\n";

            return;
        }

        json config_json;

        try
        {
            // try reading and parsing json config file provided
            std::ifstream i(config_json_path);
            i >> config_json;

            db_config = config_json["database_test"];
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error reading confing file "
                 << config_json_path << ": "
                 << e.what() << '\n';
            throw e;
        }

        xmreg::MySqlConnector::url      = db_config["url"];
        xmreg::MySqlConnector::port     = db_config["port"];
        xmreg::MySqlConnector::username = db_config["user"];
        xmreg::MySqlConnector::password = db_config["password"];
        xmreg::MySqlConnector::dbname   = db_config["dbname"];

        try
        {
            auto conn = std::make_shared<xmreg::MySqlConnector>(
                    new mysqlpp::MultiStatementsOption(true));

            // MySqlAccounts will try connecting to the mysql database
            xmr_accounts = std::make_shared<xmreg::MySqlAccounts>(conn);
        }
        catch(std::exception const& e)
        {
            std::cerr << e.what() << '\n';
            throw  e;
        }

        db_data = xmreg::read("../sql/openmonero_test.sql");
    }

    virtual void SetUp()
    {

        mysqlpp::Query query = xmr_accounts
                ->get_connection()->get_connection().query(db_data);

        query.parse();

        try
        {
            bool is_success = query.exec();

            for (size_t i = 1; query.more_results(); ++i) {
                is_success = query.store_next();
            }
        }
        catch (std::exception& e)
        {
            std::cerr << e.what() << '\n';
            throw  e;
        }

    }

    static std::string db_data;
    static json db_config;
    static std::shared_ptr<xmreg::MySqlAccounts> xmr_accounts;
};

std::string MYSQL_TEST::db_data;
json MYSQL_TEST::db_config;
std::shared_ptr<xmreg::MySqlAccounts> MYSQL_TEST::xmr_accounts;


TEST_F(MYSQL_TEST, Connection)
{
    EXPECT_TRUE(xmr_accounts != nullptr);
    EXPECT_TRUE(xmr_accounts->get_connection()->get_connection().connected());
    EXPECT_TRUE(xmr_accounts->get_connection()->ping());
}

TEST_F(MYSQL_TEST, InsertAndGetAccount)
{

    uint64_t mock_current_blockchain_height    = 452145;
    uint64_t mock_current_blockchain_timestamp = 1529302789;

    mysqlpp::DateTime blk_timestamp_mysql_format
            = mysqlpp::DateTime(static_cast<std::time_t>(mock_current_blockchain_timestamp));

    // address to insert
    std::string xmr_addr {"44AFFq5kSiGBoZ4NMDwYtN18obc8AemS33DBLWs3H7otXft3XjrpDtQGv7SqSsaBYBb98uNbr2VBBEt7f2wfn3RVGQBEP3A"};
    std::string view_key {"f359631075708155cc3d92a32b75a7d02a5dcf27756707b47a2b31b21c389501"};
    std::string view_key_hash {"cdd3ae89cbdae1d14b178c7e7c6ba380630556cb9892bd24eb61a9a517e478cd"};

    int64_t acc_id = xmr_accounts->insert(xmr_addr, view_key_hash,
                                  blk_timestamp_mysql_format,
                                  mock_current_blockchain_height);


    EXPECT_EQ(acc_id, 130);

    xmreg::XmrAccount acc;

    bool is_success = xmr_accounts->select(xmr_addr, acc);

    EXPECT_EQ(acc.id, 130);
    EXPECT_EQ(acc.scanned_block_height, mock_current_blockchain_height);
    EXPECT_EQ(acc.scanned_block_timestamp, mock_current_blockchain_timestamp);
    EXPECT_EQ(acc.viewkey_hash, view_key_hash);

}


TEST_F(MYSQL_TEST, GetAccount)
{

    // existing address
    std::string xmr_addr {"57Hx8QpLUSMjhgoCNkvJ2Ch91mVyxcffESCprnRPrtbphMCv8iGUEfCUJxrpUWUeWrS9vPWnFrnMmTwnFpSKJrSKNuaXc5q"};

    xmreg::XmrAccount acc;

    bool is_success = xmr_accounts->select(xmr_addr, acc);

    EXPECT_TRUE(is_success);

    EXPECT_EQ(acc.id, 129);
    EXPECT_EQ(acc.scanned_block_height, 96806);
    EXPECT_EQ(acc.viewkey_hash, "1acf92d12101afe2ce7392169a38d2d547bd042373148eaaab323a3b5185a9ba");
}


