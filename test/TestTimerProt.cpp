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

struct MyTimerInfo : TimerInfo
{
   U32 getTimerLen(const TimerId) const
   {
      return 10;
   }
};

enum: Status
{
    ERROR1 = failStatus(200),
    ERROR2,
    ERROR3,
    ERROR4
};

#define EVENT(n) SimpleEventInfo(n)

#define TIMER_EVENT(n) EVENT(n + 400)

struct TimerProt1: testing::Test
{
protected:
    MyTimerInfo timerInfo;

    __transaction
    (  __timer_prot(1, __wait(1))
    )trans;

//    __transaction
//    ( __sequential
//        ( __wait(1)
//        , __throw(ERROR1)
//        , __wait(2))
//    )trans;

private:
    void SetUp() override
    {
        trans.updateTimerInfo(timerInfo);
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    void TearDown() override
    {

    }
};

// @test(id="event-1")
TEST_F(TimerProt1, after_recv_event1_should_return_TSL_SUCCESS)
{
    ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(EVENT(1)));
}

// @test(id="timeout")
TEST_F(TimerProt1, after_recv_timer1_should_return_TIMEDOUT)
{
    ASSERT_EQ(TSL_TIMEDOUT, trans.handleEvent(TIMER_EVENT(1)));
}

// @test(depends="timeout")
TEST_F(TimerProt1, after_timeout_if_recv_event1_should_return_UNKNOWN_EVENT)
{
    ASSERT_EQ(TSL_TIMEDOUT, trans.handleEvent(TIMER_EVENT(1)));
    ASSERT_EQ(TSL_UNKNOWN_EVENT, trans.handleEvent(EVENT(1)));
}

// @test(depends="event-1")
TEST_F(TimerProt1, after_recv_event1_if_timeout_should_return_UNKNOWN_EVENT)
{
    ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(EVENT(1)));
    ASSERT_EQ(TSL_UNKNOWN_EVENT, trans.handleEvent(TIMER_EVENT(1)));
}

TEST_F(TimerProt1, if_stop_should_return_stop_cause)
{
    ASSERT_EQ(ERROR2, trans.stop(ERROR2));
}

struct TimerProt2: testing::Test
{
protected:
    MyTimerInfo timerInfo;

    __transaction
    ( __timer_prot(1, __wait(1), ERROR1)
    )trans;
private:
    void SetUp() override
    {
        trans.updateTimerInfo(timerInfo);
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    void TearDown() override
    {

    }
};

TEST_F(TimerProt2, after_recv_timer1_should_return_ERROR1)
{
    ASSERT_EQ(ERROR1, trans.handleEvent(TIMER_EVENT(1)));
}

TEST_F(TimerProt2, if_stop_should_return_stop_cause)
{
    ASSERT_EQ(ERROR2, trans.stop(ERROR2));
}
