//
// Created by mwo on 11/07/18.
//

#ifndef OPENMONERO_THREADRAII_H
#define OPENMONERO_THREADRAII_H

#include <thread>
#include <iostream>

#include <boost/fiber/all.hpp>

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

template <typename T>
class FiberRAII
{
    public:

    FiberRAII(std::unique_ptr<T> _functor)
        : f {std::move(_functor)}, 
        fbr {std::ref(*f)}
    {
        fbr.detach();
    }

    FiberRAII(FiberRAII&& other)
        : f {std::move(other.f)},
          fbr {std::move(other.fbr)}
    {
        std::cout << "FiberRAII(FiberRAII&& other)" << std::endl;
    };
    //FiberRAII& operator=(FiberRAII&&) = default;
    //virtual ~FiberRAII() {
        //std::cout << "virtual ~FiberRAII() " << std::endl;
        //if (fbr.joinable())
            //fbr.join();
    //};
    virtual ~FiberRAII() = default;

    T& get_functor() {return *f;}

    protected:
        std::unique_ptr<T> f;
        boost::fibers::fiber fbr;
};

}

#endif //OPENMONERO_THREADRAII_H
