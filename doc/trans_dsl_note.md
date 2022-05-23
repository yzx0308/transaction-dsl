# 概念：

## 同步操作

任何无需进一步等待后续消息的 `Action` ,都称作 同步操作(`Synchronous Action`)。这包括:函数调用,以及只发消息、不等回应的操作。在`dsl`中，它们都需要通过 `__sync` 来指明。

用户定义同步操作需要继承自`SyncAction`。

```cpp
DEFINE_ROLE(SyncAction)
{
	ABSTRACT(cub::Status exec(const TransactionInfo&));
};
```



## 异步操作

任何需要等待异步消息的操作,都称作 异步操作(Asynchronous Action)。这包括:典型的 请求——应答操作,消息触发的操作等。而在`dsl`中，异步操作则需要通过 `__asyn` 来说明。

用户定义异步操作需要继承自`SimpleAsyncAction`。

```cpp
DEFINE_ROLE(SyncAction)
{
	ABSTRACT(cub::Status exec(const TransactionInfo&));
};

struct Action : SyncAction
{
    ABSTRACT(cub::Status handleEvent(const TransactionInfo&, const ev::Event&));
    ABSTRACT(void   kill(const TransactionInfo&, const cub::Status cause));
};

struct SimpleAsyncAction: Action{...}
```



## SchedAction

`SchedAction`为可以被`dsl`调度的操作，需要满足如下接口约束：

```cpp
DEFINE_ROLE(FinalAction)
{
   ABSTRACT(cub::Status exec(TransactionContext&));
   ABSTRACT(cub::Status handleEvent(TransactionContext&, const ev::Event&));
   ABSTRACT(void   kill(TransactionContext&, const cub::Status));
};

struct SchedAction : FinalAction
{
   ABSTRACT(cub::Status stop(TransactionContext&, const cub::Status cause));
};
```

在`dsl`中，可以调度的操作，都必须是`SchedAction`类型。

但是在使用中用户自定义的操作类型分别为：`SyncAction`和`SimpleAsyncAction`，不满足`dsl`的要求。

如何使之满足？答案是**适配器模式**。

异步操作(类型为`Action`)由`ACTION__`适配成`SchedAction`：

```cpp
   struct SchedActionAdapter: SchedAction{...};

   template<typename T_ACTION>
   struct ACTION__: tsl::SchedActionAdapter
   {
       IMPL_ROLE_WITH_VAR(Action, T_ACTION);
   };

   template <typename T_ASYN_ACTION>
   struct ASYN__ : ACTION__<T_ASYN_ACTION>
   {
   };
```



同步操作(类型为`SyncAction`)先适配成异步操作(`Action`)，然后再适配成`SchedAction`

```cpp
   struct SyncActionAdapter: Action{...};
   template<typename T_SYNC_ACTION>
   struct GenericSyncAtionAdpater: tsl::SyncActionAdapter
   {
   private:
       IMPL_ROLE_WITH_VAR(SyncAction, T_SYNC_ACTION);
   };

   template <typename T_SYNC_ACTION>
   struct SYNC__ : ACTION__<GenericSyncAtionAdpater<T_SYNC_ACTION> >
   {
   };
```



## ActionThread

`ActionThread`对`SchedAction`进行了状态管理。

实际上`ActionThread`并不是当作`SchedAction`来使用。

这里继承`SchedAction`从语义上应该为能力约束，强制要求`ActionThread`需实现对应的接口，控制权限应该为`private`。

```cpp
struct ActionThread : private SimpleRuntimeContext, SchedAction
{
  ...
private:
   USE_ROLE(SchedAction);
};
```



# 代码梳理

| 元素/概念     | 类型                                                         | 输入参数类型 | 备注                 |
| ------------- | ------------------------------------------------------------ | ------------ | -------------------- |
| TRANSACTION__ | SchedulerBasedTransaction--><br />SchedTransaction-->Transaction | ActionThread |                      |
| PROCEDURE__   | Procedure--><br />SchedAction                                | SchedAction  |                      |
| TIMER_PROT__  | Procedure-><br />SchedAction                                 | SchedAction  |                      |
| SEQUENTIAL__  | SchedSequentialAction--><br />SchedAction                    | SchedAction  |                      |
| WAIT__        | SchedWaitAction--><br />SchedAction                          | EventId      |                      |
| THREAD__      | ActionThread--><br />SchedAction                             | SchedAction  | 做了action的状态管理 |
| ASYN__        | SchedActionAdapter-->SchedAction                             | Action       |                      |
| SYNC__        | SchedActionAdapter-->SchedAction                             | SyncAction   |                      |





# 技术点梳理

## DCI



## 适配器模式



## 状态模式



## 转换构造函数



# 物理设计



# 常用接口

|      接口       | 用途、行为 |
| :-------------: | :--------: |
|      start      |            |
|   handleEvent   |            |
|      stop       |            |
| updateTimerInfo |            |
|      kill       |            |



# handleEvent返回值

| 返回值        | 语意                          |
| ------------- | ----------------------------- |
| SUCCESS       | 此操作成功结束                |
| CONTINUE      | 操作尚未结束,需要进一步的处理 |
| UNKNOWN_EVENT | 当前消息不是自己期待的消息    |
| 错误码        | 失败,并已经中止               |



# 参考文献

1. [C++适配器模式](https://blog.csdn.net/lvxu666/article/details/120940663)