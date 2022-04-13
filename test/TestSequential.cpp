/*
 * TestTransaction.cpp
 *
 *  Created on: 2022年4月10日
 *      Author: daniel
 */
#include "gtest/gtest.h"

#include "trans-dsl/TransactionDSL.h"
#include "event/impl/SimpleEventInfo.h"
#include "event/concept/Event.h"
#include "trans-dsl/action/SimpleAsyncActionHelper.h"
#include <event/impl/ConsecutiveEventInfo.h>
#include <trans-dsl/TslStatus.h>
#include "trans-dsl/action/Actor.h"

using namespace cub;
using namespace tsl;
using namespace ev;

namespace cub
{
    extern void log_error(const char* file,
                            unsigned int line,
                            const char* fmt,
                            ...){}
}

enum: Status
{
    ERROR1 = failStatus(200),
    ERROR2,
    ERROR3,
    ERROR4
};

struct MyContext: private SimpleRuntimeContext, SimpleTransactionContext
{
    MyContext()
    : SimpleTransactionContext((RuntimeContext&) *this)
    {
    }
};

#define EVENT(n) SimpleEventInfo(n)

// @fixture(tags="seq")
struct Sequential: testing::Test
{
protected:
    __transaction
    ( __sequential
    ( __wait(1)
    , __wait(2))
    )trans;
private:
    void SetUp() override
    {
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    void TearDown() override
    {

    }
};

/*SUCCESS:trans 成功结束
 *CONTINUE:trans 仍在工作
 *UNKNOWN_EVENT:trans收到一个未期待的消息
 *错误码:trans失败并且终止
 * */
TEST_F(Sequential,after_recv_event1_should_return_TSL_CONTINUE)
{
    ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
}

TEST_F(Sequential,after_recv_event2_should_return_UNKNOWN_EVENT)
{
    ASSERT_EQ(TSL_UNKNOWN_EVENT, trans.handleEvent(EVENT(2)));
}

TEST_F(Sequential,after_recv_event1_if_recv_event2_should_return_TSL_SUCCESS)
{
    ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
    ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(EVENT(2)));
}

TEST_F(Sequential,after_stop_should_return_TSL_SUCCESS)
{
    ASSERT_EQ(TSL_FORCE_STOPPED, trans.stop());
}

TEST_F(Sequential,after_stop_if_recv_event1_should_return_UNKNOWN_EVENT)
{
    ASSERT_EQ(TSL_FORCE_STOPPED, trans.stop());
    ASSERT_EQ(TSL_UNKNOWN_EVENT, trans.handleEvent(EVENT(1)));
}

TEST_F(Sequential,after_kill_if_recv_event1_should_return_UNKNOWN_EVENT)
{
    trans.kill();
    ASSERT_EQ(TSL_UNKNOWN_EVENT, trans.handleEvent(EVENT(1)));
}

// @fixture(tags="seq throw")
struct Sequential2: testing::Test
{
protected:
    __transaction
    ( __sequential
        ( __wait(1)
        , __throw(ERROR1)
        , __wait(2))
    )trans;
private:
    void SetUp() override
    {
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    void TearDown() override
    {

    }
};

TEST_F(Sequential2, after_recv_event1_should_return_ERROR1)
{
    ASSERT_EQ(ERROR1, trans.handleEvent(EVENT(1)));
}

TEST_F(Sequential2, after_recv_event2_should_return_UNKNOWN_EVENT)
{
    ASSERT_EQ(TSL_UNKNOWN_EVENT, trans.handleEvent(EVENT(2)));
}

TEST_F(Sequential2, after_recv_event1_if_recv_event2_should_return_UNKNOWN_EVENT)
{
    ASSERT_EQ(ERROR1, trans.handleEvent(EVENT(1)));
    ASSERT_EQ(TSL_UNKNOWN_EVENT, trans.handleEvent(EVENT(2)));
}

