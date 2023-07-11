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

namespace cub
{
    extern void log_error(const char* file, unsigned int line, const char* fmt, ...)
    {
    }
}

enum ErrorNo : Status
{
    ERROR1 = failStatus(200),
    ERROR2,
    ERROR3,
    ERROR4
};

struct MyContext: private SimpleRuntimeContext, SimpleTransactionContext
{
    MyContext() : SimpleTransactionContext((RuntimeContext&) *this)
    {
    }
};

#define EVENT(n) SimpleEventInfo(n)

// @fixture(tags="seq")
FIXTURE(Sequential)
{
    __transaction
    ( __sequential
    ( __wait(1)
    , __wait(2))
    )trans;

    SETUP()
    {
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    TEST("after recv event-1, should return TSL_CONTINUE")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
    }

    TEST("after recv event-2, should return UNKNOWN_EVENT")
    {
        ASSERT_EQ(TSL_UNKNOWN_EVENT, trans.handleEvent(EVENT(2)));
    }

    TEST("after recv event-1, if recv event-2, should return TSL_SUCCESS")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
        ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(EVENT(2)));
    }

    TEST("after stop, should return TSL_SUCCESS")
    {
        ASSERT_EQ(TSL_FORCE_STOPPED, trans.stop());
    }

    TEST("after stop, if recv event-1, should return UNKNOWN_EVENT")
    {
        ASSERT_EQ(TSL_FORCE_STOPPED, trans.stop());
        ASSERT_EQ(TSL_UNKNOWN_EVENT, trans.handleEvent(EVENT(1)));
    }

    TEST("after kill, if recv event-1, should return UNKNOWN_EVENT")
    {
        trans.kill();
        ASSERT_EQ(TSL_UNKNOWN_EVENT, trans.handleEvent(EVENT(1)));
    }
};

// @fixture(tags="seq throw")
FIXTURE(Sequential2)
{
    __transaction
    ( __sequential
    ( __wait(1)
    , __throw(ERROR1)
    , __wait(2))
    )trans;

    SETUP()
    {
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    TEST("after recv event-1, should return ERROR1")
    {
        ASSERT_EQ(ERROR1, trans.handleEvent(EVENT(1)));
    }

    TEST("after recv event-2, should return UNKNOWN_EVENT")
    {
        ASSERT_EQ(TSL_UNKNOWN_EVENT, trans.handleEvent(EVENT(2)));
    }

    TEST("after recv event-1, if recv event-2, should return UNKNOWN_EVENT")
    {
        ASSERT_EQ(ERROR1, trans.handleEvent(EVENT(1)));
        ASSERT_EQ(TSL_UNKNOWN_EVENT, trans.handleEvent(EVENT(2)));
    }
};

FIXTURE(Safe)
{
    __transaction
    ( __safe(__wait(1))
    )trans;

    SETUP()
    {
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    // @test(id="stop")
    TEST("after started, if stop, should return TSL_CONTINUE")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.stop(ERROR1));
    }

    // @test(depends="stop")
    TEST("after stopped, if received event-1, should return TSL_SUCCESS")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.stop(ERROR1));
        ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(EVENT(1)));
    }

    TEST("after started, if received event-1, should return TSL_SUCCESS")
    {
        ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(EVENT(1)));
    }
};

FIXTURE(Unstop1)
{
    __transaction
    ( __sequential
    ( __safe(__wait(1))
    , __wait(2))
    )trans;

    SETUP()
    {
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    // @test(id="stop")
    TEST("after started, if stop, should return TSL_CONTINUE")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.stop(ERROR1));
    }

    // @test(depends="stop")
    TEST("after stopped, if received event-1, should return stop cause")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.stop(ERROR1));
        ASSERT_EQ(ERROR1, trans.handleEvent(EVENT(1)));
    }

    // @test(id="event-1")
    TEST("after started, if received event-1, should return TSL_CONTINUE")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
    }

    // @test(depends="event-1")
    TEST("after recv event-1, if recv event-2, should return TSL_SUCCESS")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
        ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(EVENT(2)));
    }
};

FIXTURE(Concurrent2)
{
    __transaction
    ( __prot_procedure
    (__concurrent
    ( __procedure
    (__sequential
    ( __wait(1)
    , __throw(ERROR1))
    , __finally(__wait(2)))
    , __wait(3)))
    )trans;

    SETUP()
    {
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    // @test(id="event-1")
    TEST("after recv event-1, should return TSL_CONTINUE")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
    }

    // @test(depends="event-1")
    TEST("after recv event-1, if recv event-3, should return UNKNOWN_EVENT")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
        ASSERT_EQ(TSL_UNKNOWN_EVENT, trans.handleEvent(EVENT(3)));
    }

    TEST("after start, if recv event-3, should return TSL_CONTINUE")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(3)));
    }
};

FIXTURE(Void)
{
    __transaction
    ( __void(__wait(1))
    )trans;

    SETUP()
    {
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    TEST("after started, if stop, should return TSL_SUCCESS, instead of stop cause")
    {
        ASSERT_EQ(TSL_SUCCESS, trans.stop(ERROR1));
    }
};

FIXTURE(Status1)
{
    __transaction
    ( __throw(ERROR1)
    , __finally
    ( __on_status
    ( ERROR1
    , __procedure
    ( __throw(TSL_SUCCESS)
    , __finally(__on_succ(__wait(1))))))
    )trans;

    // @test(id="start")
    TEST("after start, should return TSL_CONTINUE")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    // @test(depends="start")
    TEST("after recv event-1, should return ERROR1")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.start());
        ASSERT_EQ(ERROR1, trans.handleEvent(EVENT(1)));
    }
};

FIXTURE(Status2)
{
    __transaction
    ( __throw(ERROR1)
    , __finally
    ( __on_fail
    ( __procedure
    ( __throw(TSL_SUCCESS)
    , __finally(__on_fail(__wait(1))))))
    )trans;

    TEST("after start, should return ERROR1")
    {
        ASSERT_EQ(ERROR1, trans.start());
    }
};

FIXTURE(StopOnFinally)
{
    __transaction
    ( __procedure
    ( __nop()
    , __finally(__wait(1)))
    , __finally(__on_fail(__throw(ERROR1)))
    )trans;

    SETUP()
    {
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    // @test(id="stop")
    TEST("after start, if stop, should return TSL_CONTINUE")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.stop());
    }

    // @test(depends="stop")
    TEST("after stop, if recv event-1, should return TSL_SUCCESS")
    {
        ASSERT_EQ(TSL_CONTINUE, trans.stop());
        ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(EVENT(1)));
    }
};

FIXTURE(Peek)
{
    __transaction
    ( __sequential(__peek(1), __wait(1))
    )trans;

    SETUP()
    {
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    TEST("after recv event-1, should return TSL_SUCCESS")
    {
        ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(EVENT(1)));
    }
};

FIXTURE(safe_mode)
{
    __transaction
    ( __sequential
    ( __wait(1)
    , __safe_mode(__wait(2))
    , __wait(3))
    )trans;

    SETUP()
    {
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    TEST("after start, if stop, should return stop cause")
    {
       ASSERT_EQ(ERROR1, trans.stop(ERROR1));
    }

    // @test(id="ev-1")
    TEST("after started, if recv event-1, should return TSL_CONTINUE")
    {
       ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
    }

    // @test(id="stop", depends="ev-1")
    TEST("after recv event-1, if stop, should return TSL_CONTINUE")
    {
       ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));

       ASSERT_EQ(TSL_CONTINUE, trans.stop(ERROR1));
    }

    // @test(id="safe", depends="stop")
    TEST("after stop, is recv event-2, should return TSL_CONTINUE")
    {
       ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
       ASSERT_EQ(TSL_CONTINUE, trans.stop(ERROR1));

       ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(2)));
    }

    // @test(depends="safe")
    TEST("after safe mode, if stop, should return stop cause")
    {
       ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
       ASSERT_EQ(TSL_CONTINUE, trans.stop(ERROR1));
       ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(2)));

       ASSERT_EQ(ERROR1, trans.stop(ERROR1));
    }

    // @test(depends="safe")
    TEST("after safe mode, if recv event-3, should return TSL_SUCCESS")
    {
       ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(1)));
       ASSERT_EQ(TSL_CONTINUE, trans.stop(ERROR1));
       ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(EVENT(2)));

       ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(EVENT(3)));
    }
};

FIXTURE(void_status)
{
    __transaction
    ( __throw(ERROR1)
    , __finally(__on_fail(__void(__on_fail(__wait(1)))))
    )trans;

    SETUP()
    {
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    TEST("after start, if recv event-1, should return ERROR1")
    {
        ASSERT_EQ(ERROR1, trans.handleEvent(EVENT(1)));
    }
};

struct Foo
{

};

struct ReleaseRrm: SyncAction
{
    Status exec(const TransactionInfo& trans)
    {
        //Foo* foo = ACTOR();
        return TSL_SUCCESS;
    }

private:
    USE_ACTOR(Foo);
};

DEF_ACTOR_BASED_ACTION(Foo, ReleaseRrm);

struct Rrm1
{
    Foo* operator()(const TransactionInfo&)
    {
        return 0;
    }
};

struct Rrm2
{
    Foo* operator()(const TransactionInfo&)
    {
        return 0;
    }
};

FIXTURE(Actor)
{
    __def(Frag, __params(__actor(ACTOR)))__as
    ( __sync(ReleaseRrm__onto<ACTOR>)
    );

    __transaction
    ( __sequential
    ( __apply(Frag, __with(Rrm1))
    , __sync(ReleaseRrm__onto<Rrm1>))
    )trans;

    TEST("after start, should return TSL_SUCCESS")
    {
        ASSERT_EQ(TSL_SUCCESS, trans.start());
    }
};
