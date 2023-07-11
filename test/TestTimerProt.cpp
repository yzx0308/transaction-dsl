#include <cut/cut.hpp>
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

USING_CUM_NS

struct MyTimerInfo: TimerInfo
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
};
#define EVENT(n) SimpleEventInfo(n)
#define TIMER_EVENT(n) EVENT(n + 400)

enum ErrorNo : Status
{
    ERROR1 = failStatus(200),
    ERROR2,
    ERROR3,
    ERROR4
};

FIXTURE(TimerProt1)
{
    MyTimerInfo timerInfo;

    __transaction
    ( __timer_prot(1, __wait(1))
    )trans;

    SETUP()
    {
        trans.updateTimerInfo(timerInfo);
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    TEST("after recv event-1, should return TSL_SUCCESS")
    {
        ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(EVENT(1)));
    }

    TEST("after recv timer-1, should return TIMEDOUT")
    {
        ASSERT_EQ(TSL_TIMEDOUT, trans.handleEvent(TIMER_EVENT(1)));
    }

    TEST("after timeout, if recv event-1, should return UNKNOWN_EVENT")
    {
        ASSERT_EQ(TSL_TIMEDOUT, trans.handleEvent(TIMER_EVENT(1)));
        ASSERT_EQ(TSL_UNKNOWN_EVENT, trans.handleEvent(EVENT(1)));
    }

    TEST("after recv event-1, if timeout, should return UNKNOWN_EVENT")
    {
        ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(EVENT(1)));
        ASSERT_EQ(TSL_UNKNOWN_EVENT, trans.handleEvent(TIMER_EVENT(1)));
    }

    TEST("if stop, should return stop cause")
    {
        ASSERT_EQ(ERROR2, trans.stop(ERROR2));
    }
};

const TimerId TIMER1 = 1;
const TimerId TIMER2 = 2;
const ev::EventId EVENT1 = 1;
const ev::EventId EVENT2 = 2;

FIXTURE(TimerProt2)
{
    MyTimerInfo timerInfo;

    __transaction
    (__sequential( // 一定要加sequential
      __timer_prot(TIMER1, __wait(EVENT1), ERROR1)
     ,__timer_prot(TIMER2, __wait(EVENT2)))
    )trans;


    SETUP()
    {
        trans.updateTimerInfo(timerInfo);
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    TEST("after recv event-1 and event-2 ,should return TSL_SUCCESS")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(EVENT1)));
        ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(EVENT(EVENT2)));
    }

    TEST("after recv timer-1, should return ERROR1")
    {
        ASSERT_EQ(ERROR1, trans.handleEvent(TIMER_EVENT(TIMER1)));
    }

    TEST("if stop, should return stop cause")
    {
        ASSERT_EQ(ERROR2, trans.stop(ERROR2));
    }
};

FIXTURE(TimerProt3)
{
    MyTimerInfo timerInfo;

    __transaction
    ( __timer_prot(1, __sequential(__wait(1), __wait(2)))
    )trans;

    SETUP()
    {
       trans.updateTimerInfo(timerInfo);
       ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    // @test(id="event-1")
    TEST("after recv event-1, should return TSL_CONTINUE")
    {
       ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
    }

    // @test(id="timeout")
    TEST("after recv timer-1, should return TIMEDOUT")
    {
       ASSERT_EQ(TSL_TIMEDOUT, trans.handleEvent(TIMER_EVENT(1)));
    }

    // @test(depends="event-1")
    TEST("after recv timer-1, should return TIMEDOUT")
    {
       ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
       ASSERT_EQ(TSL_TIMEDOUT, trans.handleEvent(TIMER_EVENT(1)));
    }

    TEST("if stop, should return stop cause")
    {
       ASSERT_EQ(ERROR2, trans.stop(ERROR2));
    }

    // @test(depends="event-1")
    TEST("if stop, should return stop cause")
    {
       ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
       ASSERT_EQ(ERROR2, trans.stop(ERROR2));
    }

    // @test(id="event-1-2", depends="event-1")
    TEST("after recv event-1, if recv event-2, should return TSL_SUCCESS")
    {
       ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
       ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(EVENT(2)));
    }

    // @test(depends="event-1-2")
    TEST("after recv event-1-2, if stop, should return TSL_SUCCESS")
    {
       ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
       ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(EVENT(2)));
       ASSERT_EQ(TSL_SUCCESS, trans.stop(ERROR2));
    }

    // @test(depends="event-1-2")
    TEST("after recv event-1-2, if timeout, should return UNKNOWN_EVENT")
    {
       ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
       ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(EVENT(2)));
       ASSERT_EQ(TSL_UNKNOWN_EVENT, trans.handleEvent(TIMER_EVENT(1)));
    }
};

FIXTURE(TimerProt4)
{
    MyTimerInfo timerInfo;

    __transaction
    ( __timer_prot(1, __sequential(__wait(1), __throw(ERROR1)))
    )trans;

    SETUP()
    {
        trans.updateTimerInfo(timerInfo);
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    // @test(id="event-1")
    TEST("after recv event-1, should return ERROR1")
    {
        ASSERT_EQ(ERROR1, trans.handleEvent(EVENT(1)));
    }
};

FIXTURE(TimerProt5)
{
    MyTimerInfo timerInfo;

    __transaction
    ( __timer_prot(1, __procedure
    ( __throw(ERROR1)
    , __finally(__wait(1))))
    )trans;

    SETUP()
    {
        trans.updateTimerInfo(timerInfo);
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    TEST("after recv event-1, should return ERROR1")
    {
        ASSERT_EQ(ERROR1, trans.handleEvent(EVENT(1)));
    }

    // @test(id="timeout")
    TEST("after recv timer-1, should return TSL_CONTINUE")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(TIMER_EVENT(1)));
    }

    // @test(depends="timeout")
    TEST("after timeout, if recv event-1, should return ERROR1")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(TIMER_EVENT(1)));
        ASSERT_EQ(ERROR1, trans.handleEvent(EVENT(1)));
    }

    // @test(id="stop")
    TEST("if stop, should return TSL_CONTINUE")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.stop(ERROR2));
    }

    // @test(depends="stop")
    TEST("after stop, if recv event-1, should return ERROR1")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.stop(ERROR2));
        ASSERT_EQ(ERROR1, trans.handleEvent(EVENT(1)));
    }
};

FIXTURE(TimerProt6)
{
    MyTimerInfo timerInfo;

    __transaction
    ( __timer_prot(1, __procedure
    ( __wait(1)
    , __finally(__throw(ERROR1))))
    )trans;

    SETUP()
    {
        trans.updateTimerInfo(timerInfo);
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    TEST("after recv event-1, should return ERROR1")
    {
        ASSERT_EQ(ERROR1, trans.handleEvent(EVENT(1)));
    }

    // @test(id="stop")
    TEST("if stop, should return ERROR1")
    {
        ASSERT_EQ(ERROR1, trans.stop(ERROR2));
    }

    TEST("after recv timer-1, should return ERROR1")
    {
        ASSERT_EQ(ERROR1, trans.handleEvent(TIMER_EVENT(1)));
    }
};

FIXTURE(TimerProt7)
{
    MyTimerInfo timerInfo;

    __transaction
    ( __timer_prot(1, __procedure
    ( __wait(1)
    , __finally(__wait(2))))
    )trans;

    SETUP()
    {
        trans.updateTimerInfo(timerInfo);
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    TEST("after recv event-1, should return TSL_CONTINUE")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
    }

    // @test(id="stop")
    TEST("if stop, should return TSL_CONTINUE")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.stop(ERROR2));
    }

    // @test(depends="stop")
    TEST("after stop, if recv event-2, should return ERROR2")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.stop(ERROR2));
        ASSERT_EQ(ERROR2, trans.handleEvent(EVENT(2)));
    }

    // @test(id="timeout")
    TEST("after recv timer-1, should return TSL_CONTINUE")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(TIMER_EVENT(1)));
    }

    // @test(depends="timeout")
    TEST("after timeout, if recv event-2, should return TIMEDOUT")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(TIMER_EVENT(1)));
        ASSERT_EQ(TSL_TIMEDOUT, trans.handleEvent(EVENT(2)));
    }
};

