#include <cut/cut.hpp>
#include "trans-dsl/TransactionDSL.h"
#include "event/impl/SimpleEventInfo.h"
#include "event/concept/Event.h"
#include "trans-dsl/action/SimpleAsyncActionHelper.h"
#include <event/impl/ConsecutiveEventInfo.h>
#include <trans-dsl/TslStatus.h>
#include "trans-dsl/action/Actor.h"
#include <assert.h>
#include <iostream>
#include <trans-dsl/sched/trans/GenericTransaction.h>
using namespace cub;
using namespace tsl;
using namespace ev;
using namespace std;
USING_CUM_NS

/*
 *实现pdf中面试的例子;
 * */

struct Event1
{
    int num;
};

DEF_SIMPLE_ASYNC_ACTION(ApplicationAcceptance)
{
    Status exec(const TransactionInfo&)
    {
        WAIT_ON(1, handleEvent1); // 可以理解为等待请求
        cout<<"ASYNC Action1::exec"<<endl;
        return TSL_CONTINUE;
    }

    ACTION_SIMPLE_EVENT_HANDLER_DECL(handleEvent1, Event1);
};

ACTION_SIMPLE_EVENT_HANDLER_DEF(ApplicationAcceptance, handleEvent1, Event1)
{
    return TSL_SUCCESS;
}

DEF_SIMPLE_ASYNC_ACTION(BackgroundInvestigation)
{
    Status exec(const TransactionInfo&)
    {
        WAIT_ON(1, handleEvent1); // 可以理解为等待请求
        cout<<"ASYNC Action1::exec"<<endl;
        return TSL_CONTINUE;
    }

    ACTION_SIMPLE_EVENT_HANDLER_DECL(handleEvent1, Event1);
};

ACTION_SIMPLE_EVENT_HANDLER_DEF(BackgroundInvestigation, handleEvent1, Event1)
{
    return TSL_SUCCESS;
}

DEF_SIMPLE_ASYNC_ACTION(Exam)
{
    Status exec(const TransactionInfo&)
    {
        WAIT_ON(1, handleEvent1); // 可以理解为等待请求
        cout<<"ASYNC Action1::exec"<<endl;
        return TSL_CONTINUE;
    }

    ACTION_SIMPLE_EVENT_HANDLER_DECL(handleEvent1, Event1);
};

ACTION_SIMPLE_EVENT_HANDLER_DEF(Exam, handleEvent1, Event1)
{
    return TSL_SUCCESS;
}


DEF_SIMPLE_ASYNC_ACTION(Interview)
{
    Status exec(const TransactionInfo&)
    {
        WAIT_ON(1, handleEvent1); // 可以理解为等待请求
        cout<<"ASYNC Action1::exec"<<endl;
        return TSL_CONTINUE;
    }

    ACTION_SIMPLE_EVENT_HANDLER_DECL(handleEvent1, Event1);
};

ACTION_SIMPLE_EVENT_HANDLER_DEF(Interview, handleEvent1, Event1)
{
    return TSL_SUCCESS;
}

DEF_SIMPLE_ASYNC_ACTION(OfferNegotiation)
{
    Status exec(const TransactionInfo&)
    {
        WAIT_ON(1, handleEvent1); // 可以理解为等待请求
        cout<<"ASYNC Action1::exec"<<endl;
        return TSL_CONTINUE;
    }

    ACTION_SIMPLE_EVENT_HANDLER_DECL(handleEvent1, Event1);
};

ACTION_SIMPLE_EVENT_HANDLER_DEF(OfferNegotiation, handleEvent1, Event1)
{
    return TSL_SUCCESS;
}

DEF_SIMPLE_ASYNC_ACTION(OnBoard)
{
    Status exec(const TransactionInfo&)
    {
        WAIT_ON(1, handleEvent1); // 可以理解为等待请求
        cout<<"ASYNC Action1::exec"<<endl;
        return TSL_CONTINUE;
    }

    ACTION_SIMPLE_EVENT_HANDLER_DECL(handleEvent1, Event1);
};

ACTION_SIMPLE_EVENT_HANDLER_DEF(OnBoard, handleEvent1, Event1)
{
    return TSL_SUCCESS;
}

struct TransTimer: TimerInfo
{
    U32 getTimerLen(const TimerId timerId) const
    {
        if(timerId == 1)
        {
            return 10;
        }
        if (timerId == 2)
        {
            return 20;
        }
        return 30;
    }

    static TransTimer& getInstance()
    {
        static TransTimer timer;
        return timer;
    }
};

UNKNOWN_INTERFACE(Context0, 0x08311757)
{
    int getValue()
    {
        return 5;
    }
};

UNKNOWN_INTERFACE(Context1, 0x08311748)
{
    int getValue()
    {
        return 3;
    }
};

struct  TransContext
        : com::Unknown
        , Context0
        , Context1
{
    BEGIN_INTERFACE_TABLE()
    __HAS_INTERFACE(Context0)
    __HAS_INTERFACE(Context1)
    END_INTERFACE_TABLE()
};

__def_transaction
(__sequential(
    __asyn(ApplicationAcceptance)
   ,__concurrent(
         __asyn(BackgroundInvestigation)
        ,__sequential(
            __asyn(Exam)
           ,__asyn(Interview)))
   ,__asyn(OfferNegotiation)
   ,__timer_prot(1, __asyn(OnBoard))))ApplicationAcceptanceTrans;

FIXTURE(TestOffer1)
{
    SETUP()
    {
        GenericTransaction<TransTimer, ApplicationAcceptanceTrans, TransContext, TransactionListener>  trans(1);
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    // @test(id="once")
    TEST("after recv event-1 once, should return TSL_CONTINUE")
    {
       // ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
    }
};
