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
 *��HR�ĽǶ�������һ�����Ե�����
 * 1.����������Ͷ�ݵļ���
 * 2.��������
 * 3.���ű���
 * 4.��������
 * 5.Э��offer
 * 6.��ְ
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
        printf("�յ���ѡ�˼��� name[%s], age[%d]\n", pInfo->name, pInfo->age);

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
        printf("���󱳾����鸺���� ���к�ѡ�� name[%s], age[%d]�ı�������\n", info.name, info.age);
        return TSL_CONTINUE;
    }

    cub::Status handleEvent1(const TransactionInfo& transInfo, const ev::Event& event)
    {
        assert(event.getEventId() == BACKGROUND_INVESTIGATION_RSP);
        printf("��������ͨ��\n");
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
        printf("֪ͨ���Ը�����,���ź�ѡ��[%s]�ı���\n", info.name);
        return TSL_CONTINUE;
    }

    cub::Status handleEvent1(const TransactionInfo& transInfo, const ev::Event& event)
    {
        assert(event.getEventId() == ARRANGE_EXAM_RSP);
        PersonCtx* personCtx = transInfo.toRole<PersonCtx>();
        const PersonInfo& personInfo = personCtx->getPersonInfo();

        ExamResult* pExam = (ExamResult*)event.getMsg();

        printf("��ѡ��name[%s] ���Գɼ�[%d]\n", pExam->name, pExam->score);
        string ctxName(personInfo.name);
        string examName(pExam->name);
        assert(ctxName == examName); // daniel

        ExamResultCtx* ctx = transInfo.toRole<ExamResultCtx>();
        ctx->saveResult(*pExam);
        if(pExam->score > 60)
        {
            printf("����ͨ��\n");
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
        printf("֪ͨ���Ը�����,���ź�ѡ��[%s]������\n", info.name);
        return TSL_CONTINUE;
    }

    cub::Status handleEvent1(const TransactionInfo& transInfo, const ev::Event& event)
    {
        assert(event.getEventId() == ARRANGE_INTERVIEW_RSP);
        PersonCtx* ctx = transInfo.toRole<PersonCtx>();
        const PersonInfo& info = ctx->getPersonInfo();
        printf("��ѡ��[%s]����ͨ��\n", info.name);
        return TSL_SUCCESS;
    }
};


DEF_SIMPLE_ASYNC_ACTION(OfferNegotiation)
{
    Status exec(const TransactionInfo&)
    {
        WAIT_ON(OFFERE_NEGOTITATION_RSP, handleEvent1); // �������Ϊ�ȴ�����
        printf("Э��offer\n");
        return TSL_CONTINUE;
    }

    cub::Status handleEvent1(const TransactionInfo& transInfo, const ev::Event& event)
    {
        printf("����offer\n");
        assert(event.getEventId() == OFFERE_NEGOTITATION_RSP);
        return TSL_SUCCESS;
    }
};


DEF_SIMPLE_ASYNC_ACTION(OnBoard)
{
    Status exec(const TransactionInfo&)
    {
        WAIT_ON(START_WORK_NOTIFY, handleEvent1);
        printf("�ȴ���ѡ����ְ\n");
        return TSL_CONTINUE;
    }

    cub::Status handleEvent1(const TransactionInfo& transInfo, const ev::Event& event)
    {
        assert(event.getEventId() == START_WORK_NOTIFY);
        printf("��ѡ����ְ��\n");
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
        ////////////////��ѡ��Ͷ�ݼ���////////////////////////
        PersonInfo info;
        info.age = 28;
        memset(&info.name, 0, sizeof(info.name));
        strcat(info.name, "daniel");
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(ConsecutiveEventInfo(POST_RESUME_REQ, info)));

        /////////////////����������///////////////////////
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(SimpleEventInfo(BACKGROUND_INVESTIGATION_RSP)));

        /////////////////���Խ��/////////////////////
        ExamResult result;
        memset(&result.name, 0, sizeof(result.name));
        strcat(result.name, "daniel");
        result.score = 95;
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(ConsecutiveEventInfo(ARRANGE_EXAM_RSP, result)));

        /////////////////���Խ��///////////////////////
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(SimpleEventInfo(ARRANGE_INTERVIEW_RSP)));


        /////////////////����offer/////////////////////
        ASSERT_EQ(TSL_CONTINUE, trans.handleEvent(SimpleEventInfo(OFFERE_NEGOTITATION_RSP)));

        /////////////////��ְ//////////////////////
        ASSERT_EQ(TSL_SUCCESS, trans.handleEvent(SimpleEventInfo(START_WORK_NOTIFY)));
    }
};
