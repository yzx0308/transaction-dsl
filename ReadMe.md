# 编译：

编译前需要安装如下依赖库：

1. [CUB : C++ Unified Base Library](https://github.com/yanxicheung/cub)
2. [Event](https://github.com/yanxicheung/event)



安装完成后，如下方式编译`transaction-dsl`

```shell
cd transaction-dsl
mkdir build
cd build
cmake ..
make
```

生成的库文件路径：`transaction-dsl/build/src/libtrans-dsl.a`



# 安装：

```shell
sudo make install
```

头文件默认安装在：`/usr/local/include/trans-dsl`

库文件默认安装在：`/usr/local/lib/libtrans-dsl.a`



# 测试用例：

出于以下原因，移除了原作者的用例：

1. 原作者漏提交了文件，编译失败。
2. 原作者用了多种测试框架（如`test-ng-pp`），安装比较麻烦。

这里修改了原作者的用例，改用[cut](https://github.com/horance-liu/cut)测试框架，如需运行测试用例，请提前安装好。



按照如下方式编译、运行测试用例：

```shell
cd build
cmake -DENABLE_TEST=1 ..
make
./test/trans-test
```



常用`cut`命令：

| 命令     | 用途                                  | 示例                                                         |
| -------- | ------------------------------------- | ------------------------------------------------------------ |
| --help   | 查看cut提供的命令                     | ./trans-test --help                                          |
| --list   | 查看所有用例，但不运行他们            | ./trans-test --list                                          |
| --filter | 指定运行哪部分测试用例 支持简单的正则 | 执行Sequential fixture：<br />./trans-test  --filter="Sequential::.*"<br />执行指定测试case：<br />./trans-test  --filter="proc1::should be able to stop" |



# 其他：

1. `transaction-dsl`的详细文档位于`doc`文件夹下。