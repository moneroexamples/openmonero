//
// Created by mwo on 11/07/18.
//

#include "ThreadRAII.h"

namespace xmreg
{

ThreadRAII::ThreadRAII(std::thread&& _t, DtorAction _action)
    : t {std::move(_t)}, action {_action}
{}

ThreadRAII::~ThreadRAII()
{
    if (t.joinable())
    {
        if (action == DtorAction::join)
        {
            //std::cout << "\nThreadRAII::~ThreadRAII() t.join()\n"
            //          << std::endl;
            t.join();
        }
        else
        {
            //std::cout << "\nThreadRAII::~ThreadRAII() t.detach()\n"
            //          << std::endl;
            t.detach();            
        }
    }
}

}
