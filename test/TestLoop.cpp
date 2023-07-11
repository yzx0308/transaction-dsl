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

#define EVENT(n) SimpleEventInfo(n)
#define TIMER_EVENT(n) EVENT(n + 400)

enum ErrorNo : Status
{
    ERROR1 = failStatus(200),
    ERROR2,
    ERROR3,
    ERROR4
};


FIXTURE(Loop0)
{
    struct ShouldExecute
    {
        ShouldExecute()
        : times(0)
        {
        }

        bool operator()(const TransactionInfo&)
        {
            return times++ < 2;
        }

        int times;
    };

    __transaction
    ( __loop0(ShouldExecute, __prot_procedure(__wait(1)))
    )trans;

    SETUP()
    {
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    // @test(id="once")
    TEST("after recv event-1 once, should return TSL_CONTINUE")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
    }

    TEST("after recv event-1 twice, should return TSL_SUCCESS")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
        ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(EVENT(1)));
    }

    TEST("after start, if stop, should return stop cause")
    {
        ASSERT_EQ(ERROR1, trans.stop(ERROR1));
    }

    // @test(depends="once")
    TEST("after recv event-1 once, if stop, should return TSL_SUCCESS")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
        ASSERT_EQ(TSL_SUCCESS, trans.stop(ERROR1));
    }

};

FIXTURE(Loop0Extra)
{
    struct ShouldExecute
    {
        ShouldExecute()
        : times(0)
        {
        }

        bool operator()(const TransactionInfo&)
        {
            return times++ < 2;
        }

        int times;
    };

    __transaction
    ( __loop0(ShouldExecute, __wait(1))
    )trans;

    SETUP()
    {
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    // @test(id="once")
    TEST("after recv event-1 once, should return TSL_CONTINUE")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
    }

    TEST("after recv event-1 twice, should return TSL_SUCCESS")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
        ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(EVENT(1)));
    }

    TEST("after start, if stop, should return stop cause")
    {
        ASSERT_EQ(ERROR1, trans.stop(ERROR1));
    }

    // @test(depends="once")
    TEST("after recv event-1 once, if stop, should return stop cause")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
        ASSERT_EQ(ERROR1, trans.stop(ERROR1));
    }
};

FIXTURE(Loop1)
{
    struct ShouldExecute
    {
        ShouldExecute()
        : times(0)
        {
        }

        bool operator()(const TransactionInfo&)
        {
            return times++ < 2;
        }

        int times;
    };

    __transaction
    ( __loop1(ShouldExecute, __prot_procedure(__wait(1)))
    )trans;

    SETUP()
    {
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    // @test(id="once")
    TEST("after recv event-1 once, should return TSL_CONTINUE")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
    }

    // @test(id="twice")
    TEST("after recv event-1 twice, should return TSL_CONTINUE")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
    }

    TEST("after recv event-1 3 times, should return TSL_SUCCESS")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
        ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(EVENT(1)));
    }

    TEST("after start, if stop, should return stop cause")
    {
        ASSERT_EQ(ERROR1, trans.stop(ERROR1));
    }

    // @test(depends="once")
    TEST("after recv event-1 once, if stop, should return stop cause")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
        ASSERT_EQ(ERROR1, trans.stop(ERROR1));
    }

    // @test(depends="twice")
    TEST("after recv event-1 twice, if stop, should return TSL_SUCCESS")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
        ASSERT_EQ(TSL_SUCCESS, trans.stop(ERROR1));
    }
};

FIXTURE(Loop1Extra)
{
    struct ShouldExecute
    {
        ShouldExecute()
        : times(0)
        {
        }

        bool operator()(const TransactionInfo&)
        {
            return times++ < 2;
        }

        int times;
    };

    __transaction
    ( __loop1(ShouldExecute, __wait(1))
    )trans;

    SETUP()
    {
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    // @test(id="once")
    TEST("after recv event-1 once, should return TSL_CONTINUE")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
    }

    // @test(id="twice")
    TEST("after recv event-1 twice, should return TSL_CONTINUE")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
    }

    TEST("after recv event-1 3 times, should return TSL_SUCCESS")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
        ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(EVENT(1)));
    }

    TEST("after start, if stop, should return stop cause")
    {
        ASSERT_EQ(ERROR1, trans.stop(ERROR1));
    }

    // @test(depends="once")
    TEST("after recv event-1 once, if stop, should return stop cause")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
        ASSERT_EQ(ERROR1, trans.stop(ERROR1));
    }

    // @test(depends="twice")
    TEST("after recv event-1 twice, if stop, should return stop cause")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
        ASSERT_EQ(ERROR1, trans.stop(ERROR1));
    }
};

