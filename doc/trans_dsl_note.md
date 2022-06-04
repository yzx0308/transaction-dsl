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

​         *Transaction DSL* 提供了 __procedure 来定义一个过程，无论这个过程中的所有操作全部成功，还是执行到某一步时发生了失败，都会进入结束模式。用户可以自己定义结束模式里应该执行的操作是什么。如果按照之前对于事务的描述，则用户可以在结束模式里根据过程进入结束模式时的状态，进行提交或回滚操作。

所以，_procedure 包含了两个参数：第一个参数是此过程应该执行的正常操作，第二个的参数则是以__finally 修饰的结束模式中应该执行的操作。



# 建议

​		`Transaction DSL`使用面向对象设计，对设计的理解和演进是对模型和结构的理解和调整。不要一上来就看代码。在阅读代码之前，有几件事情必须去做:其一，是手动构建一次工程，并且运行测试用例；其二，亲自动手写一个`Demo`感受以下。

​        先将工程跑起来，目的不是为了`Debug`代码，而是在于了解工程的构建方式，及其认识系统的基本结构，并体会系统的使用方式。

​		面向对象的代码看着看着很容易断，比如遇到虚接口，就跟不下去了。通常是先掌握模型和结构，然后在结构中打开某个点的代码进行查看和修改，请记住，先模型，再接口，后实现。

​		本文的梳理也是基于上述思路进行。



# 概念

介绍一些`Transaction DSL`中比较常用的概念，其他未列举的内容可以自行研究。



## SyncAction

任何无需进一步等待后续消息的 `Action` ,都称作 同步操作(`Synchronous Action`)。这包括:函数调用,以及只发消息、不等回应的操作。在`dsl`中，它们都需要通过 `__sync` 来指明。

用户定义同步操作需要继承自`SyncAction`。

```cpp
DEFINE_ROLE(SyncAction)
{
	ABSTRACT(cub::Status exec(const TransactionInfo&));
};
```



## SimpleAsyncAction

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

如何使之满足？答案是***适配器模式***。

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

`SchedAction`定义了对一个事件的处理，我们希望在此基础上对`SchedAction`的**状态**也做一些管理，但是不去修改现有的`SchedAction`的代码，怎么做？

答案是***装饰器模式***，于是就有了`ActionThread`。

```cpp
struct ActionThread : private SimpleRuntimeContext, SchedAction
{
    OVERRIDE(cub::Status exec(TransactionContext&));
    OVERRIDE(cub::Status handleEvent(TransactionContext&, const ev::Event&));
    OVERRIDE(cub::Status stop(TransactionContext&, const cub::Status));
    OVERRIDE(void   kill(TransactionContext&, const cub::Status));
    ...
private:
    USE_ROLE(SchedAction);
};
```

对比适配器模式的经典用法，会发现***没有***类从`ActionThread`派生，***并且没有对行为进行拓展***。

当然以下代码并不是我们通常理解上的派生，它的目的是创建一个`SchedAction`类型的对象，完成交织：

```cpp
template<typename T_ACTION>
struct THREAD__: ActionThread
{
    private:
    IMPL_ROLE_WITH_VAR(SchedAction, T_ACTION);
};
```

看`THREAD__`的样子是不是和前面讲的适配器模式有点一样？恩，看样子形式差不多。

不过别忘了`ActionThread`和`SchedAction`的接口是一样的，没有适配的必要，他们之间的关系是：`ActionThread`是`SchedAction`的功能的拓展。



## SchedSequentialAction

`__sequential`定义了顺序操作的步骤：

```cpp
__transaction (
__sequential
( __asyn(Action1)
, __sync(Action2)
, __asyn(Action3)
, __asyn(Action4)
, __sync(Action5))
);
```

`__sequential`的实现使用变参模板，其参数类型为`SchedAction`。

`__sequential`派生自`SchedSequentialAction`。

`SchedSequentialAction`持有一系列的`SchedAction`实例，这些实例的运行时类型可以是任何派生自`SchedAction`的子类。

```cpp
struct SchedSequentialAction: SchedAction
{
    ...
private:
    // 可以理解为list<SchedAction*> Actions;
   typedef cub::List<LinkedSchedAction> Actions;

   SchedAction* current;
   Actions actions;
   cub::Status finalStatus;
};
```

由于`SchedSequentialAction`也派生自`SchedAction`，因此`SchedSequentialAction`也可以持有`SchedSequentialAction`类型的实例，所以可以理解`__sequential`支持嵌套组合:

```cpp
   __mt_transaction
   ( __sequential
       ( __wait(1)
       , __safe_mode
           (__sequential
              ( __fork(1, __wait(2))
              , __wait(3)
              , __join(1)))
       , __wait(4))
   )trans;
```



## SchedWaitAction

当一个事务执行到某个点的时候,会期待某个事件的发生,但是,事件的内容并不重要(或干脆没有内容),此时,一个事件就像一个 `signal` 一样。`__wait` 来完成这样的事情，其参数是一个整数:

```cpp
__transaction
( ...
, __wait(EV_SOMETHING)
, ... );
```

`__wait`派生自`SchedWaitAction`

```cpp
struct SchedWaitAction : SchedAction
{
	...
}
```



## SchedConcurrentAction

一旦系统因为性能要求,需要同时给不同其它系统/子系统发出请求消息,并同时等待它们的应答。这时可以使用`__concurrent`来描述这一概念。

```cpp
__transaction(
    __sequential(
        __req(Action1)
        ,__sync(Action2)
        ,__concurrent(__asyn(Action1), __asyn(Action3))
        ,__asyn(Action1)
        ,__rsp(Action2)
    ))trans;
```

`__concurrent`的实现使用变参模板，其输入参数类型为`SchedAction`。

`__concurrent`派生自`SchedConcurrentAction`。

```cpp
struct SchedConcurrentAction : private SimpleRuntimeContext, SchedAction
{
	...
private:
    //  // 可以理解为list<ActionThread*> Actions;
   typedef cub::List<LinkedActionThread> Threads;
   typedef Threads::Iterator Thread;
   ...
};
```



## Procedure

`Transaction DSL` 提供了 `__procedure` 来定义一个过程,无论这个过程中的所有操作全部成功,还是执行到
某一步时发生了失败,都会进入结束模式。

用户可以自己定义结束模式里应该执行的操作是什么。如果按照之前对于事务的描述,则用户可以在结束模式里根据过程进入结束模式时的状态,进行提交或回滚操作。

所以,`__procedure` 包含了两个参数:

1. 第一个参数是此过程应该执行的正常操作。（类型为`SchedAction`）
2. 第二个的参数则是以__finally 修饰的结束模式中应该执行的操作。（类型为`FinalAction`）

```cpp
struct Procedure : private SimpleRuntimeContext, SchedAction
{
	...
private:
   struct State;
   State* state;

   struct AutoSwitch;
private:
   __DECL_STATE(Idle);
   __DECL_STATE(Working);
   __DECL_STATE(Stopping);
   __DECL_STATE(Final);
   __DECL_STATE(Done);

   struct WorkingState;
private:
   USE_ROLE(SchedAction);
   USE_ROLE(FinalAction);
   ABSTRACT(bool isProtected() const);
};
```



## SchedTimerProtAction

`__timer_prot`对一个异步操作的时间进行了约束:

```cpp
  __transaction
    ( __timer_prot(1, __sequential(__wait(1), __wait(2)))
    )trans;
```

`__timer_prot`包含三个参数:

1. timerID;
2. 操作;
3. 超时返回的状态，默认为`TSL_TIMEDOUT`。

从实现上来看:

```cpp
   template<tsl::TimerId TIMER_ID, typename T_ACTION, cub::Status TIMEOUT_STATUS>
   struct SchedTimerProtAction
            : private TimerProtActionState
            , TimerProtAction
            , TimerProtFinalAction
   {
      SchedTimerProtAction() : timer(TIMER_ID) {}

   private:
      IMPL_ROLE(TimerProtActionState);

   private:
      mutable PlatformSpecifiedRelativeTimer timer;
      ROLE_PROTO_TYPE(RelativeTimer)
      {
         return timer;
      }

      IMPL_ROLE_WITH_VAR(SchedAction, T_ACTION);

      OVERRIDE(cub::Status getTimeoutStatus() const)
      {
         return TIMEOUT_STATUS;
      }
   };

   template<TimerId TIMER_ID, typename T_ACTION, cub::Status TIMEOUT_STATUS = TSL_TIMEDOUT>
   struct TIMER_PROT__: Procedure
   {
   private:
      mutable SchedTimerProtAction<TIMER_ID, T_ACTION, TIMEOUT_STATUS> action;
      ROLE_PROTO_TYPE(SchedAction)
      {
         return (TimerProtAction&)action;
      }

      ROLE_PROTO_TYPE(FinalAction)
      {
         return (TimerProtFinalAction&)action;
      }

      OVERRIDE(bool isProtected() const)
      {
         return false;
      }
   };
```

`TIMER_PROT__`完成了`SchedTimerProtAction`和`Procedure`的交织。

1. `Procedure`依赖于`SchedAction`和`FinalAction`这两个接口。(**依赖倒置原则，按照接口编程**)
2. `SchedTimerProtAction`是一个多角色对象，实现了上面这两个接口，使用`ROLE_PROTO_TYPE`完成特定上下文下的角色转换功能。



## SimpleTransaction

使用`__transaction`定义一个事务，一个事务由多个`SchedAction`组成，一个事务需要满足如下接口：

```cpp
DEFINE_ROLE(Transaction)
{
   ABSTRACT(cub::Status start());
   ABSTRACT(cub::Status handleEvent(const ev::Event&));
   ABSTRACT(cub::Status stop(const StopCause& = StopCause()));
   ABSTRACT(void   kill(const StopCause& = StopCause()));

   cub::Status startWithEvent(const ev::Event&);
};
```

事务使用调度器`TransactionScheduler`对装饰成`ActionThread`的`SchedAction`进行调度：

```cpp
struct SchedTransaction : Transaction
{
	...
private:
   USE_ROLE(TransactionScheduler);
   USE_ROLE(ActionThread);
};
```



## SchedSwitchCaseAction

在一个较为复杂的事务中,一些操作,只有在一定的条件下才会执行;或者,在不同的条件下执行的时机不同。如下,`Action2` 就是一个可选的操作,它只有在系统的某个开关打开的情况下才会执行。

```cpp
auto ShouldExecAction2(const TransactionInfo&) -> bool {
	return SystemConfig::shouldExecAction2();
}
__sequential
    ( __req(Action1)
    , __optional(ShouldExecAction2, __asyn(Action2))
    , __asyn(Action3)
    , __asyn(Action4)
    , __rsp(Action5));
```

从代码中可以看出,`__optional` 有两个参数:

1. 第一个参数是一个谓词,即一个返回值是一个 bool 的函数。
2. 第二个参数一个 `Action` 将会得到执行。

这是 `C++` 解决这类问题的常用手段。如果谓词是一个 仿函数,即一个类,`Transaction DSL` 对其有着和基本操作一样的约束,即它也必须是自满足的。所以,它必须亲自访问环境来确定一个条件是否成立。



`__optional`的类型是`SchedSwitchCaseAction`

```cpp
DEFINE_ROLE(ActionPath) EXTENDS(CUB_NS::ListElem<ActionPath>)
{
   ABSTRACT(bool shouldExecute(const TransactionInfo&));
   ABSTRACT(SchedAction& getAction());
};

struct SchedSwitchCaseAction : SchedAction
{
    ...
private:
   typedef cub::List<ActionPath> Paths;

   Paths          paths;
   SchedAction*   selectedAction;
};

template <typename PRED, typename T_ACTION>
struct CASE__ : ActionPath
{
    OVERRIDE(bool shouldExecute(const TransactionInfo& info))
    {
        return pred(info);
    }

    OVERRIDE(SchedAction& getAction())
    {
        return action;
    }

    private:
    T_ACTION action;
    PRED pred;
};

template <typename PRED, typename T_ACTION>
struct OPTIONAL__
    : SWITCH__< CASE__<PRED, T_ACTION> >
    {
    };

#define __optional(...) TSL_NS::details::OPTIONAL__< __VA_ARGS__ >
```

`struct CASE__` 组合了谓词和一个`SchedAction`，`struct CASE__` 的类型是`ActionPath`。

`SWITCH__`持有一系列`ActionPath`的集合，运行时遍历每个`ActionPath`，如果发现某个`ActionPath`的谓词为真，则执行该`ActionPath`下的`SchedAction`。



## SchedLoopAction

系统提供了两种循环:

```cpp
#define __loop0(...) \
       TSL_NS::details::LOOP0__< __VA_ARGS__ >

#define __loop1(...) \
       TSL_NS::details::LOOP1__< __VA_ARGS__ >
```

`__loop0`相当于`while(cond)`。

`__loop1`相当于`do {...} while(cond)`，至少循环一次。

这两种循环都继承自`SchedLoopAction`。`SchedLoopAction`的实现如下:

```cpp
struct SchedLoopAction : SchedAction
{
	...
private:
   ABSTRACT(bool shouldExecute(TransactionContext&));
   ABSTRACT(void reset());

   USE_ROLE(SchedAction);
};
```



## TransactionContext

```cpp
struct TransactionContext : com::Unknown
{
   HAS_ROLE(TransactionInfo);
   HAS_ROLE(TransactionListener);
   HAS_ROLE(TransactionMode);
};
```

## RuntimeContext



## RuntimeContextInfo



## TransactionInfo

​		无论是谓词还是基本操作，这些需要用户定义的类，都必须是子满足的，所以，它们自身计算所需的信息都必须亲历其为的到环境中查找。由于 *Transaction* 自身也是环境的一部分，所以 *Transaction* 必须通过参数将自身的信息传递给基本操作或谓词，从而让它们有能力得到一切需要的信息，这就是 TransactionInfo 的由来。

​		TransactionInfo 是一个接口类。通过它，你首先可以获取到 **实例标识**（*Instance ID* ）。因为有些系统对于同种类型的领域对象会创建多个实例，而每个实例都可能会有自己的 *Transaction*；通过 *Instance ID* ，用户定义的类就可以知道当前的 *Transaction* 属于哪个实例。另外，*Transaction* 会通过 `TransactionInfo` 告知自身的运行时状态：是成功还是失败，如果失败，是什么原因导致的失败等等信息。

```cpp
DEFINE_ROLE(TransactionInfo)
{
   ABSTRACT(InstanceId getInstanceId() const);
   ABSTRACT(cub::Status getStatus() const);
   ABSTRACT(com::Unknown* getUserContext() const);

   template <typename ROLE>
   ROLE* toRole() const
   {
      return com::unknown_cast<ROLE>(getUserContext());
   }
};
```



# CUB库

由于 *Transaction DSL* 依赖`CUB`库，`CUB`库中有很多不错的通用代码，此处做简单梳理。



## ListElem



## List



## Placement



## Unknown

​	该接口需要被领域对象public继承。

​	能够从`com::Unknown`被转化到的目标role需要用`UNKNOWN_INTERFACE`来定义，参数是类名以及一个32位的随机数。这个随机数需要程序员自行提供，保证全局不重复（可以写一个脚本自动产生不重复的随机数，同样可以用脚本自动校验代码中已有的是否存在重复，可以把校验脚本作为版本编译检查的一部分）。

​	领域对象类继承的所有由`UNKNOWN_INTERFACE`定义的role都需要在`BEGIN_INTERFACE_TABLE()`和`END_INTERFACE_TABLE()`中由`__HAS_INTERFACE`显示注册一下（参考上面代码中`HumanObject`的写法）。最后，context持有领域对象工厂返回的`com::Unknown`指针，通过`com::unknown_cast`将其转化目标role使用，至此这种机制和`dynamic_cast`的用法基本一致，在无法完成转化的情况下会返回空指针，所以安全起见需要对返回的指针进行校验。

​	上述提供的RTTI替代手段，虽然比直接使用RTTI略显复杂，但是增加的手工编码成本并不大，带来的好处却是明显的。例如对嵌入式开发，这种机制相比RTTI来说对程序员是可控的，可以选择在仅需要该特性的范围内使用，避免无谓的内存和性能消耗。



# 技术点梳理

## DCI

见参考文献，不赘述。



## 适配器模式

已提及，不赘述。



## 装饰器模式

已提及，不赘述。

## 状态模式



## 转换构造函数



## 变参模板



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
2. [C++装饰器模式](https://www.cnblogs.com/yuandonghua/p/11842519.html)
3. [编程中的同步和异步意味着什么](https://www.elecfans.com/d/1510870.html)
4. [DCI in C++](https://www.jianshu.com/p/bb9c35606d29)
5. [状态模式](https://blog.csdn.net/m0_46655373/article/details/124037484)
6. [策略模式和状态模式的区别](https://www.csdn.net/tags/MtjaAgysNzg5MDUtYmxvZwO0O0OO0O0O.html)