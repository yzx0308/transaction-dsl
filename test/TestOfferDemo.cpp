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
#include "offerDemoProto.h"
using namespace cub;
using namespace tsl;
using namespace ev;
using namespace std;
USING_CUM_NS

/*
 *从HR的角度描述了一个面试的流程
 * 1.接收面试者投递的简历
 * 2.背景调查
 * 3.安排笔试
 * 4.安排面试
 * 5.协商offer
 * 6.入职
 *
 * */

UNKNOWN_INTERFACE(PersonCtx, 0x08311757)
{
    void updatePersonInfo(PersonInfo& info)
    {
        personInfo = info;
    }
    const PersonInfo& getPersonInfo() const
    {
        return personInfo;
    }

private:
    PersonInfo personInfo;
};

UNKNOWN_INTERFACE(ExamResultCtx, 0x08311748)
{
    void saveResult(ExamResult& result)
    {
        this->result = result;
    }
    const ExamResult& getResult() const
    {
        return result;
    }
private:
    ExamResult result;
};

struct  TransContext
        : com::Unknown
        , PersonCtx
        , ExamResultCtx
{
    BEGIN_INTERFACE_TABLE()
        __HAS_INTERFACE(PersonCtx)
        __HAS_INTERFACE(ExamResultCtx)
    END_INTERFACE_TABLE()
};

DEF_SIMPLE_ASYNC_ACTION(ApplicationAcceptance)
{
    Status exec(const TransactionInfo&)
    {
        WAIT_ON(POST_RESUME_REQ, handleEvent1);
        return TSL_CONTINUE;
    }

    cub::Status handleEvent1(const TransactionInfo& transInfo, const ev::Event& event)
    {
        assert(event.getEventId() == POST_RESUME_REQ);

        PersonInfo* pInfo = (PersonInfo*)event.getMsg();
        printf("收到候选人简历 name[%s], age[%d]\n", pInfo->name, pInfo->age);

        PersonCtx* ctx = transInfo.toRole<PersonCtx>();
        ctx->updatePersonInfo(*pInfo);

        return TSL_SUCCESS;
    }
};

DEF_SIMPLE_ASYNC_ACTION(BackgroundInvestigation)
{
    Status exec(const TransactionInfo& transInfo)
    {
        WAIT_ON(BACKGROUND_INVESTIGATION_RSP, handleEvent1);
        PersonCtx* ctx = transInfo.toRole<PersonCtx>();
        const PersonInfo& info = ctx->getPersonInfo();
        printf("请求背景调查负责人 进行候选人 name[%s], age[%d]的背景调查\n", info.name, info.age);
        return TSL_CONTINUE;
    }

    cub::Status handleEvent1(const TransactionInfo& transInfo, const ev::Event& event)
    {
        assert(event.getEventId() == BACKGROUND_INVESTIGATION_RSP);
        printf("背景调查通过\n");
        return TSL_SUCCESS;
    }
};

DEF_SIMPLE_ASYNC_ACTION(Exam)
{
    Status exec(const TransactionInfo& transInfo)
    {
        WAIT_ON(ARRANGE_EXAM_RSP, handleEvent1);
        PersonCtx* ctx = transInfo.toRole<PersonCtx>();
        const PersonInfo& info = ctx->getPersonInfo();
        printf("通知笔试负责人,安排候选人[%s]的笔试\n", info.name);
        return TSL_CONTINUE;
    }

    cub::Status handleEvent1(const TransactionInfo& transInfo, const ev::Event& event)
    {
        assert(event.getEventId() == ARRANGE_EXAM_RSP);
        PersonCtx* personCtx = transInfo.toRole<PersonCtx>();
        const PersonInfo& personInfo = personCtx->getPersonInfo();

        ExamResult* pExam = (ExamResult*)event.getMsg();

        printf("候选人name[%s] 笔试成绩[%d]\n", pExam->name, pExam->score);
        string ctxName(personInfo.name);
        string examName(pExam->name);
        assert(ctxName == examName); // daniel

        ExamResultCtx* ctx = transInfo.toRole<ExamResultCtx>();
        ctx->saveResult(*pExam);
        if(pExam->score > 60)
        {
            printf("笔试通过\n");
        }
        return TSL_SUCCESS;
    }
};

DEF_SIMPLE_ASYNC_ACTION(Interview)
{
    Status exec(const TransactionInfo& transInfo)
    {
        WAIT_ON(ARRANGE_INTERVIEW_RSP, handleEvent1);
        PersonCtx* ctx = transInfo.toRole<PersonCtx>();
        const PersonInfo& info = ctx->getPersonInfo();
        printf("通知面试负责人,安排候选人[%s]的面试\n", info.name);
        return TSL_CONTINUE;
    }

    cub::Status handleEvent1(const TransactionInfo& transInfo, const ev::Event& event)
    {
        assert(event.getEventId() == ARRANGE_INTERVIEW_RSP);
        PersonCtx* ctx = transInfo.toRole<PersonCtx>();
        const PersonInfo& info = ctx->getPersonInfo();
        printf("候选人[%s]面试通过\n", info.name);
        return TSL_SUCCESS;
    }
};


DEF_SIMPLE_ASYNC_ACTION(OfferNegotiation)
{
    Status exec(const TransactionInfo&)
    {
        WAIT_ON(OFFERE_NEGOTITATION_RSP, handleEvent1); // 可以理解为等待请求
        printf("协商offer\n");
        return TSL_CONTINUE;
    }

    cub::Status handleEvent1(const TransactionInfo& transInfo, const ev::Event& event)
    {
        printf("接收offer\n");
        assert(event.getEventId() == OFFERE_NEGOTITATION_RSP);
        return TSL_SUCCESS;
    }
};


DEF_SIMPLE_ASYNC_ACTION(OnBoard)
{
    Status exec(const TransactionInfo&)
    {
        WAIT_ON(START_WORK_NOTIFY, handleEvent1);
        printf("等待候选人入职\n");
        return TSL_CONTINUE;
    }

    cub::Status handleEvent1(const TransactionInfo& transInfo, const ev::Event& event)
    {
        assert(event.getEventId() == START_WORK_NOTIFY);
        printf("候选人入职！\n");
        return TSL_SUCCESS;
    }
};

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
    GenericTransaction<TransTimer, ApplicationAcceptanceTrans, TransContext, TransactionListener>  trans;
    SETUP()
    {
        ASSERT_EQ(TSL_CONTINUE, trans.start());
    }

    TEST("Test Offer succ")
    {
        ////////////////候选人投递简历////////////////////////
        PersonInfo info;
        info.age = 28;
        memset(&info.name, 0, sizeof(info.name));
        strcat(info.name, "daniel");
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(ConsecutiveEventInfo(POST_RESUME_REQ, info)));

        /////////////////背景调查结果///////////////////////
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(SimpleEventInfo(BACKGROUND_INVESTIGATION_RSP)));

        /////////////////笔试结果/////////////////////
        ExamResult result;
        memset(&result.name, 0, sizeof(result.name));
        strcat(result.name, "daniel");
        result.score = 95;
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(ConsecutiveEventInfo(ARRANGE_EXAM_RSP, result)));

        /////////////////面试结果///////////////////////
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(SimpleEventInfo(ARRANGE_INTERVIEW_RSP)));


        /////////////////接收offer/////////////////////
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(SimpleEventInfo(OFFERE_NEGOTITATION_RSP)));

        /////////////////入职//////////////////////
        ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(SimpleEventInfo(START_WORK_NOTIFY)));
    }
};
