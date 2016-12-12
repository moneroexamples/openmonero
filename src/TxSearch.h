//
// Created by mwo on 12/12/16.
//

#ifndef RESTBED_XMR_TXSEARCH_H
#define RESTBED_XMR_TXSEARCH_H



#include "MySqlConnector.h"
#include "tools.h"

#include <mysql++/mysql++.h>
#include <mysql++/ssqls.h>


#include <iostream>
#include <memory>
#include <thread>
#include <mutex>

namespace xmreg
{

mutex mtx;

class TxSearch
{



    bool continue_search {true};

    XmrAccount xmr_account;

public:

    TxSearch() {}

    TxSearch(XmrAccount& _acc):
            xmr_account {_acc}
    {}

    void
    search()
    {
        cout << "TxSearch::Search for: " << xmr_account.address << endl;

        while(true)
        {

            //std::lock_guard<std::mutex> lck (mtx);
            cout << " - searching tx of: " << xmr_account.address << endl;

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    void
    stop()
    {
        cout << "something to stop the thread by setting continue_search=false" << endl;
    }

    ~TxSearch()
    {
        cout << "TxSearch destroyed" << endl;
    }

};

}

#endif //RESTBED_XMR_TXSEARCH_H
