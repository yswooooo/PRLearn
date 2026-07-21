# STM32 裸机 MessageCenter Demo 设计

## 目标

使用标准 C 实现一个适用于 STM32 裸机主循环上下文的简单消息中心，让 RC 模块通过公共消息结构向 Gimbal 模块传递 `vx`、`vy`、`vw`。RC 与 Gimbal 不直接包含对方的头文件，也不访问对方的内部数据。

## 范围与约束

- 生成 `message.h`、`message.c`、`rc.c`、`gimbal.c`、`main.c`。
- 保留 PC 端回归测试 `tests/test_message.c`。
- 只使用标准 C，不引入 C++、FreeRTOS 或 STM32 HAL。
- 不使用动态内存、链表、哈希表、字符串 Topic、动态注册或宏元编程。
- 数据复制统一使用 `memcpy`；对象初始化使用 `memset`。
- 使用 `stdbool.h`、`stdint.h`、`stddef.h` 和 `string.h`。
- 对指针、消息 ID 和数据长度进行严格检查。
- 命名遵循需求指定的大驼峰、小驼峰及枚举命名方式。
- 当前实现仅用于单线程裸机主循环上下文，不保证中断与主循环并发访问安全。

## 架构

采用固定结构体存储方案。`MessageCenter` 持有一份 `MessageData` 和按 `MessageId` 索引的 `MessageTopicState`。每个 Topic 只保留最后一次成功发布的数据；新数据覆盖旧数据，读取不会删除数据或清除有效标志。

公共头文件 `message.h` 定义通信结构 `RcToGimbalMessage`、消息枚举、函数指针、发布/订阅接口结构、状态结构、数据结构和消息中心结构。RC 与 Gimbal 均只依赖该公共头文件。

## 组件职责

### message.h

声明所有公共类型、`MessageCenterInit` 和 `extern MessageCenter MessageCenterInstance`。函数指针显式接收 `struct MessageCenter *self`，以便对指定实例操作。

### message.c

定义唯一的 `MessageCenterInstance`。`MessageCenterInit` 清零对象并绑定 `Publish`、`Get`。两个静态实现函数使用 `switch (messageId)` 分派，目前只处理 `MessageId_RcToGimbal`。

发布成功时把输入复制到 `self->data.rcToGimbal`，设置 `valid`，并递增无符号 `sequence`。获取成功时把已保存的数据复制到输出对象，但不修改消息中心状态。任何非法参数、错误长度、非法 ID 或尚未发布的读取都返回 `false`，且不修改有效数据。

### rc.c

保存私有静态 `RcInstance`。`RcInit` 写入演示值；`RcTask` 构造局部 `RcToGimbalMessage`，检查发布函数指针后发布，并保存返回值以预留失败处理位置。

### gimbal.c

保存私有静态 `GimbalInstance`。`GimbalInit` 清零本地状态；`GimbalTask` 检查获取函数指针，成功读取后才更新 `targetVx`、`targetVy`、`targetVw`。

### main.c

声明 RC 和 Gimbal 的初始化与任务函数，不依赖 HAL。依次初始化消息中心、RC、Gimbal，然后在无限主循环中依次运行 RC 和 Gimbal 任务。

## 数据流

1. `RcTask` 从 RC 私有状态生成专用通信结构。
2. `Publish` 验证参数和精确数据长度，用 `memcpy` 覆盖消息中心中的最新值。
3. `GimbalTask` 调用 `Get`。
4. `Get` 验证 Topic 已发布，用 `memcpy` 把最新值复制到 Gimbal 私有状态。
5. Gimbal 根据成功获取的通信结构更新内部目标值。

`true` 或 `false` 只表示本次操作是否成功；实际消息通过输入/输出指针和 `memcpy` 传递。

## 错误处理

- `MessageCenterInit(NULL)` 直接返回。
- `Publish` 拒绝空 `self`、空输入、错误长度及非目标消息 ID。
- `Get` 拒绝空 `self`、空输出、错误长度、非目标消息 ID以及尚未成功发布的 Topic。
- RC 发布失败只保留注释形式的处理位置，不打印日志。
- Gimbal 获取失败时保留原有目标值。

## 测试与验证

`tests/test_message.c` 使用断言和真实的 `message.c` 实现验证：

- 初始化清零状态并绑定函数指针；
- 发布和获取的空指针检查；
- 精确数据长度检查；
- `MessageId_None`、`MessageId_Count` 等非法 ID 被拒绝；
- 首次发布前获取失败；
- 成功发布后可重复获取同一最新值；
- 后续发布覆盖旧值；
- 每次成功发布递增 `sequence`，失败发布不递增；
- 获取不清除 `valid` 或数据。

分别构建回归测试程序和包含五个演示源码文件的应用程序，并启用编译器严格警告。应用程序只验证编译与链接，不运行其无限循环。

## 非目标

本实现不是环形队列，不保存历史消息，不支持多订阅者消费状态，不提供中断保护、线程同步、动态 Topic 或通用序列化。
