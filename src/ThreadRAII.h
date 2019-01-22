//
// Created by mwo on 11/07/18.
//

#ifndef OPENMONERO_THREADRAII_H
#define OPENMONERO_THREADRAII_H

#include <thread>
#include <iostream>

namespace xmreg
{

// based on Mayer's ThreadRAII class (item 37)
class ThreadRAII
{
public:
    enum class DtorAction {join, detach};

    ThreadRAII(std::thread&& _t, DtorAction _action);

    ThreadRAII(ThreadRAII&&) = default;
    ThreadRAII& operator=(ThreadRAII&&) = default;

    virtual std::thread& get() {return t;}

    virtual ~ThreadRAII();

protected:
    std::thread t;
    DtorAction action;
};

template <typename T>
class ThreadRAII2 : public ThreadRAII
{
public:

    ThreadRAII2(std::unique_ptr<T> _functor,
                DtorAction _action = DtorAction::join)
        :ThreadRAII(std::thread(std::ref(*_functor)), _action),
         f {std::move(_functor)}
    {}

    T& get_functor() {return *f;}

protected:
    std::unique_ptr<T> f;
};

}

#endif //OPENMONERO_THREADRAII_H
