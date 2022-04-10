/*
 * TestTransaction.cpp
 *
 *  Created on: 2022Äê4ÔÂ10ÈÕ
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
  extern void log_error(const char* file, unsigned int line, const char* fmt, ...)
  {
  }
}

enum : Status
{
    ERROR1 = failStatus(200),
    ERROR2,
    ERROR3,
    ERROR4
};


struct MyContext : private SimpleRuntimeContext, SimpleTransactionContext
{
   MyContext() : SimpleTransactionContext((RuntimeContext&)*this)
   {
   }
};

#define EVENT(n) SimpleEventInfo(n)

struct Sequential:testing::Test
{
protected:
    __transaction
    ( __sequential
        ( __wait(1)
        , __wait(2))
    )trans;
};

TEST_F(Sequential,should_equal)
{

}

