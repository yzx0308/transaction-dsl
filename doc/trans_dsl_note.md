# 动机

1. 用于梳理 *Transaction DSL* 的代码和使用到的技术，为后续工作提供参考。
2. 摘录pdf中比较关键的内容，方便快速查找。



# 摘录

​          事务模型用来描述状态之间的转换过程；他可以由一系列的同步异步操作组成。

​          *Transaction DSL* 则是一种用来描述事务的语言。它用来定义状态之间的复杂转换过程。从而避免使用状态机来描述状态转换过程中由于异步而导致的 **临时中间状态**。

​         *Transaction DSL* 不是为了取代 **状态模型**，而是为了提供一种方法，以解决那些本来不应该属于状态模型，却在使用状态模型进行解决的问题。从而大大简化实现的复杂度，并缩小用户视图和实现视图之间的距离，让设计和实现更加符合事情的原貌，最终降低开发和维护成本。

​         你可以选择任何一个 start 接口来启动一个事务。像一个异步操作一样，如果其返回值是 SUCCESS，说明此事务已经成功的执行；如果其返回值是一个 **错误值**，则说明此事务已经失败；而如果其返回了CONTINUE ，则说明此事务正在工作状态中，尚未结束，仍然需要进一步的消息激励。

​          在 start 接口返回 CONTINUE 的情况下，随后每次系统收到一个消息，都需要调用其 handleEvent 接口，直到其返回 SUCCESS 或一个 **错误值**为止。



​          **不要在同步操作中使用时间约束，因为形同虚设**

​          虽然你可以在一段 *Transaction DSL* 代码中，对一个同步操作进行时间约束，但事实上，这个约束形同虚设。这是因为，一旦进入一个 **同步操作**的执行，事务框架就失去了控制权。所以，它无法在定时器过期时，抢占或打断一个同步操作的执行。而 **异步操作**则不同，由于它们需要等待消息激励，在等待期间，事务掌握着控制权，当收到其定时器过期消息时，事务可以马上中止其运行。



## TransactionInfo

​           无论是谓词还是基本操作，这些需要用户定义的类，都必须是子满足的，所以，它们自身计算所需的信息都必须亲历其为的到环境中查找。由于 *Transaction* 自身也是环境的一部分，所以 *Transaction* 必须通过参数将自身的信息传递给基本操作或谓词，从而让它们有能力得到一切需要的信息，这就是 TransactionInfo 的由来。

​           TransactionInfo 是一个接口类。通过它，你首先可以获取到 **实例标识**（*Instance ID* ）。因为有些系统对于同种类型的领域对象会创建多个实例，而每个实例都可能会有自己的 *Transaction*；通过 *Instance ID* ，用户定义的类就可以知道当前的 *Transaction* 属于哪个实例。另外，*Transaction* 会通过 TransactionInfo 告知自身的运行时状态：是成功还是失败，如果失败，是什么原因导致的失败等等信息。



## 异常处理

​         *Transaction DSL* 提供了 __procedure 来定义一个过程，无论这个过程中的所有操作全部成功，还是执行到某一步时发生了失败，都会进入结束模式。用户可以自己定义结束模式里应该执行的操作是什么。如果按照之前对于事务的描述，则用户可以在结束模式里根据过程进入结束模式时的状态，进行提交或回滚操作。

所以，_procedure 包含了两个参数：第一个参数是此过程应该执行的正常操作，第二个的参数则是以__finally 修饰的结束模式中应该执行的操作。



# 概念

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
2. [编程中的同步和异步意味着什么](https://www.elecfans.com/d/1510870.html)
3. [DCI in C++](https://www.jianshu.com/p/bb9c35606d29)