#include <cut/cut.hpp>
#include "trans-dsl/TransactionDSL.h"
#include "event/impl/SimpleEventInfo.h"
#include "event/concept/Event.h"
#include "trans-dsl/action/SimpleAsyncActionHelper.h"
#include <event/impl/ConsecutiveEventInfo.h>
#include <trans-dsl/TslStatus.h>
#include "trans-dsl/action/Actor.h"
#include <assert.h>
using namespace cub;
using namespace tsl;
using namespace ev;

USING_CUM_NS

struct Event1
{
    int num;
};

struct Event3
{
    int num1;
    int num2;
};
////////////////////////////////////////////////
DEF_SIMPLE_ASYNC_ACTION(Action1)
{
    Status exec(const TransactionInfo&)
    {
        WAIT_ON(1, handleEvent1); // 可以理解为等待请求
        return TSL_CONTINUE;
    }

    ACTION_SIMPLE_EVENT_HANDLER_DECL(handleEvent1, Event1);
};

ACTION_SIMPLE_EVENT_HANDLER_DEF(Action1, handleEvent1, Event1)
{
    assert(event.num == 100);  // 处理请求;
    return TSL_SUCCESS;
}

///////////////////////////////////////////////////////
struct Action2: SyncAction
{
    Status exec(const TransactionInfo&)
    {
        return TSL_SUCCESS;
    }
};

//////////////////////////////////////////////////////
DEF_SIMPLE_ASYNC_ACTION(Action3)
{
    Status exec(const TransactionInfo&)
    {
        // 可以在这里发送请求
        WAIT_ON(3, handleEvent3);// 等待响应

        return TSL_CONTINUE;
    }

    ACTION_SIMPLE_EVENT_HANDLER_DECL(handleEvent3, Event3);
};

ACTION_SIMPLE_EVENT_HANDLER_DEF(Action3, handleEvent3, Event3)
{
    assert(event.num1 == 101); // 处理响应;
    assert(event.num2 == 102);
    return TSL_SUCCESS;
}
//////////////////////////////////////////////////////

FIXTURE(AsynAction1)
{
    __transaction
    ( __sequential(__asyn(Action1), __sync(Action2))
    )trans;

    SETUP()
    {
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    TEST("after received event-1, should return TSL_SUCCESS")
    {
        Event1 event1;
        event1.num = 100;
        ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(ConsecutiveEventInfo(1, event1)));
    }
};

FIXTURE(AsynAction2)
{
    __transaction
    ( __sequential(__asyn(Action1), __sync(Action2), __asyn(Action3))
    )trans;

    SETUP()
    {
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    TEST("after received event-1, and event-3 ,should return TSL_SUCCESS")
    {
        Event1 event1;
        event1.num = 100;
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(ConsecutiveEventInfo(1, event1)));

        Event3 event3;
        event3.num1 = 101;
        event3.num2 = 102;
        ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(ConsecutiveEventInfo(3, event3)));
    }
};

