/*
 * offerDemoProto.h
 *
 *  Created on: 2022年6月4日
 *      Author: daniel
 */

#ifndef H4DD43690_15E7_474E_9C86_3C22935640FF
#define H4DD43690_15E7_474E_9C86_3C22935640FF

struct Event1
{
    int num;
};

struct PersonInfo
{
    char name[100];
    int age;
};

struct ExamResult
{
    char name[100];
    int score;
};
//消息定义
#define POST_RESUME_REQ 1000

#define BACKGROUND_INVESTIGATION_REQ 1001
#define BACKGROUND_INVESTIGATION_RSP 1002

#define ARRANGE_EXAM_REQ 1003
#define ARRANGE_EXAM_RSP 1004

#define ARRANGE_INTERVIEW_REQ 1005
#define ARRANGE_INTERVIEW_RSP 1006

#define OFFERE_NEGOTITATION_REQ 1007
#define OFFERE_NEGOTITATION_RSP 1008

#define START_WORK_NOTIFY   1009

#endif /* H4DD43690_15E7_474E_9C86_3C22935640FF */
