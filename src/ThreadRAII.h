//
// Created by mwo on 11/07/18.
//

#ifndef OPENMONERO_PINGTHREAD_H
#define OPENMONERO_PINGTHREAD_H

#include <thread>

namespace xmreg
{

// based on Mayer's ThreadRAII class (item 37)
class ThreadRAII
{
public:
    enum class DtorAction { join, detach};

    ThreadRAII(std::thread&& _t, DtorAction _action);

    ThreadRAII(ThreadRAII&&) = default;
    ThreadRAII& operator=(ThreadRAII&&) = default;

    std::thread& get() {return t;}

    ~ThreadRAII();

private:
    std::thread t;
    DtorAction action;
};

}

#endif //OPENMONERO_PINGTHREAD_H
