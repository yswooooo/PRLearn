# STM32 Bare-Metal MessageCenter Demo Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement and verify a standard-C latest-value MessageCenter demo that transfers `vx`, `vy`, and `vw` from an RC module to a Gimbal module without HAL or direct module coupling.

**Architecture:** `message.h` owns the shared contract, while `message.c` owns the only global center instance and fixed per-topic storage. `rc.c` publishes a dedicated DTO, `gimbal.c` copies the latest DTO into private state, and `main.c` demonstrates the bare-metal main-loop call order. Host tests use the real implementation and assertions.

**Tech Stack:** ISO C11, standard headers only, GCC 8.1.0 host compiler, MinGW runtime assertions.

## Global Constraints

- Use standard C only; do not introduce C++, FreeRTOS, STM32 HAL, or dynamic memory.
- Do not use `malloc`, `calloc`, `free`, linked lists, hash tables, string topics, dynamic registration, or macro metaprogramming.
- Use `memcpy` for all message data copying and `memset` for object clearing.
- Use `stdbool.h`, `stdint.h`, `stddef.h`, and `string.h` where required.
- Validate null pointers and require exact `sizeof(RcToGimbalMessage)` lengths.
- Preserve the exact public names specified in the design and add clear Doxygen-style Chinese comments.
- Support main-loop use only; document that interrupt/main-loop concurrent access is unsupported.

---

### Task 1: MessageCenter contract and latest-value storage

**Files:**
- Create: `tests/test_message.c`
- Create: `message.h`
- Create: `message.c`

**Interfaces:**
- Consumes: ISO C headers only.
- Produces: `MessageCenterInit(MessageCenter *self)`, global `MessageCenterInstance`, and the `Publish`/`Get` function-pointer interfaces defined by `message.h`.

- [ ] **Step 1: Write the failing MessageCenter behavior test**

```c
#include "message.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static void TestInitialization(void)
{
    MessageCenter center;
    size_t messageIndex;

    memset(&center, 0xA5, sizeof(center));
    MessageCenterInit(&center);

    assert(center.Pub.Publish != NULL);
    assert(center.Sub.Get != NULL);
    for (messageIndex = 0U; messageIndex < (size_t)MessageId_Count; ++messageIndex)
    {
        assert(center.topicState[messageIndex].valid == false);
        assert(center.topicState[messageIndex].sequence == 0U);
    }

    MessageCenterInit(NULL);
}

static void TestValidation(void)
{
    MessageCenter center;
    RcToGimbalMessage inputData = {1.0F, 2.0F, 3.0F};
    RcToGimbalMessage outputData = {9.0F, 8.0F, 7.0F};
    RcToGimbalMessage unchangedOutput = outputData;

    MessageCenterInit(&center);

    assert(center.Pub.Publish(NULL, MessageId_RcToGimbal,
                              &inputData, sizeof(inputData)) == false);
    assert(center.Pub.Publish(&center, MessageId_RcToGimbal,
                              NULL, sizeof(inputData)) == false);
    assert(center.Pub.Publish(&center, MessageId_RcToGimbal,
                              &inputData, sizeof(inputData) - 1U) == false);
    assert(center.Pub.Publish(&center, MessageId_None,
                              &inputData, sizeof(inputData)) == false);
    assert(center.Pub.Publish(&center, MessageId_Count,
                              &inputData, sizeof(inputData)) == false);
    assert(center.topicState[MessageId_RcToGimbal].sequence == 0U);

    assert(center.Sub.Get(NULL, MessageId_RcToGimbal,
                          &outputData, sizeof(outputData)) == false);
    assert(center.Sub.Get(&center, MessageId_RcToGimbal,
                          NULL, sizeof(outputData)) == false);
    assert(center.Sub.Get(&center, MessageId_RcToGimbal,
                          &outputData, sizeof(outputData) - 1U) == false);
    assert(center.Sub.Get(&center, MessageId_None,
                          &outputData, sizeof(outputData)) == false);
    assert(center.Sub.Get(&center, MessageId_Count,
                          &outputData, sizeof(outputData)) == false);
    assert(center.Sub.Get(&center, MessageId_RcToGimbal,
                          &outputData, sizeof(outputData)) == false);
    assert(memcmp(&outputData, &unchangedOutput, sizeof(outputData)) == 0);
}

static void TestLatestValueMailbox(void)
{
    MessageCenter center;
    RcToGimbalMessage firstInput = {1.25F, -2.5F, 0.75F};
    RcToGimbalMessage secondInput = {-4.0F, 5.5F, 6.25F};
    RcToGimbalMessage outputData = {0.0F, 0.0F, 0.0F};

    MessageCenterInit(&center);

    assert(center.Pub.Publish(&center, MessageId_RcToGimbal,
                              &firstInput, sizeof(firstInput)) == true);
    assert(center.topicState[MessageId_RcToGimbal].valid == true);
    assert(center.topicState[MessageId_RcToGimbal].sequence == 1U);

    firstInput.vx = 99.0F;
    assert(center.Sub.Get(&center, MessageId_RcToGimbal,
                          &outputData, sizeof(outputData)) == true);
    assert(outputData.vx == 1.25F);
    assert(outputData.vy == -2.5F);
    assert(outputData.vw == 0.75F);

    outputData.vx = 100.0F;
    assert(center.Sub.Get(&center, MessageId_RcToGimbal,
                          &outputData, sizeof(outputData)) == true);
    assert(outputData.vx == 1.25F);
    assert(center.topicState[MessageId_RcToGimbal].valid == true);
    assert(center.topicState[MessageId_RcToGimbal].sequence == 1U);

    assert(center.Pub.Publish(&center, MessageId_RcToGimbal,
                              &secondInput, sizeof(secondInput)) == true);
    assert(center.topicState[MessageId_RcToGimbal].sequence == 2U);
    assert(center.Sub.Get(&center, MessageId_RcToGimbal,
                          &outputData, sizeof(outputData)) == true);
    assert(memcmp(&outputData, &secondInput, sizeof(outputData)) == 0);
}

int main(void)
{
    TestInitialization();
    TestValidation();
    TestLatestValueMailbox();
    return 0;
}
```

- [ ] **Step 2: Compile the test and verify RED**

Run:

```powershell
gcc -std=c11 -Wall -Wextra -Werror -pedantic -I. tests/test_message.c message.c -o D:\TMP\message_center_test.exe
```

Expected: compilation fails because `message.h` and `message.c` do not exist.

- [ ] **Step 3: Implement the shared contract**

Create `message.h`:

```c
#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @brief RC 发送给 Gimbal 的专用通信数据。 */
typedef struct
{
    float vx;
    float vy;
    float vw;
} RcToGimbalMessage;

/** @brief 消息通道标识，可直接用作状态数组下标。 */
typedef enum
{
    MessageId_None = 0,
    MessageId_RcToGimbal,
    MessageId_Count
} MessageId;

struct MessageCenter;

/** @brief 发布函数签名。 */
typedef bool (*MessagePublishFunction)(
    struct MessageCenter *self,
    MessageId messageId,
    const void *inputData,
    size_t dataSize);

/** @brief 获取函数签名。 */
typedef bool (*MessageGetFunction)(
    struct MessageCenter *self,
    MessageId messageId,
    void *outputData,
    size_t dataSize);

/** @brief 消息发布接口。 */
typedef struct
{
    MessagePublishFunction Publish;
} MessagePublisher;

/** @brief 消息订阅接口。 */
typedef struct
{
    MessageGetFunction Get;
} MessageSubscriber;

/** @brief 单个消息通道的状态。 */
typedef struct
{
    bool valid;
    uint32_t sequence;
} MessageTopicState;

/** @brief 消息中心保存的全部最新消息数据。 */
typedef struct
{
    RcToGimbalMessage rcToGimbal;
} MessageData;

/**
 * @brief 最新值消息中心。
 * @note 仅适用于主循环上下文，不支持中断与主循环并发访问。
 */
typedef struct MessageCenter
{
    MessagePublisher Pub;
    MessageSubscriber Sub;
    MessageTopicState topicState[MessageId_Count];
    MessageData data;
} MessageCenter;

/** @brief 全局唯一消息中心实例。 */
extern MessageCenter MessageCenterInstance;

/** @brief 初始化消息中心对象。 */
void MessageCenterInit(MessageCenter *self);

#endif
```

- [ ] **Step 4: Implement latest-value publish and get behavior**

Create `message.c`:

```c
#include "message.h"

#include <string.h>

static bool MessageCenterPublishImpl(
    MessageCenter *self,
    MessageId messageId,
    const void *inputData,
    size_t dataSize);

static bool MessageCenterGetImpl(
    MessageCenter *self,
    MessageId messageId,
    void *outputData,
    size_t dataSize);

MessageCenter MessageCenterInstance;

void MessageCenterInit(MessageCenter *self)
{
    if (self == NULL)
    {
        return;
    }

    memset(self, 0, sizeof(*self));
    self->Pub.Publish = MessageCenterPublishImpl;
    self->Sub.Get = MessageCenterGetImpl;
}

static bool MessageCenterPublishImpl(
    MessageCenter *self,
    MessageId messageId,
    const void *inputData,
    size_t dataSize)
{
    if ((self == NULL) || (inputData == NULL))
    {
        return false;
    }

    switch (messageId)
    {
        case MessageId_RcToGimbal:
            if (dataSize != sizeof(RcToGimbalMessage))
            {
                return false;
            }
            memcpy(&self->data.rcToGimbal, inputData, dataSize);
            self->topicState[messageId].valid = true;
            self->topicState[messageId].sequence++;
            return true;

        default:
            return false;
    }
}

static bool MessageCenterGetImpl(
    MessageCenter *self,
    MessageId messageId,
    void *outputData,
    size_t dataSize)
{
    if ((self == NULL) || (outputData == NULL))
    {
        return false;
    }

    switch (messageId)
    {
        case MessageId_RcToGimbal:
            if (dataSize != sizeof(RcToGimbalMessage))
            {
                return false;
            }
            if (!self->topicState[messageId].valid)
            {
                return false;
            }
            memcpy(outputData, &self->data.rcToGimbal, dataSize);
            return true;

        default:
            return false;
    }
}
```

- [ ] **Step 5: Compile and run the test to verify GREEN**

Run:

```powershell
gcc -std=c11 -Wall -Wextra -Werror -pedantic -I. tests/test_message.c message.c -o D:\TMP\message_center_test.exe
D:\TMP\message_center_test.exe
```

Expected: compilation and execution both exit with code `0` and no warnings.

- [ ] **Step 6: Commit the MessageCenter core**

```powershell
git add message.h message.c tests/test_message.c
git commit -m "feat: implement latest-value message center"
```

---

### Task 2: RC publisher and Gimbal subscriber modules

**Files:**
- Create: `tests/test_modules.c`
- Create: `rc.c`
- Create: `gimbal.c`

**Interfaces:**
- Consumes: `MessageCenterInit`, `MessageCenterInstance`, `MessageId_RcToGimbal`, and `RcToGimbalMessage` from Task 1.
- Produces: `RcInit(void)`, `RcTask(void)`, `GimbalInit(void)`, and `GimbalTask(void)`.

- [ ] **Step 1: Write the failing module-flow test**

```c
#include "message.h"

#include <assert.h>

#include "../rc.c"
#include "../gimbal.c"

int main(void)
{
    MessageCenterInit(&MessageCenterInstance);
    RcInit();
    GimbalInit();

    assert(GimbalInstance.targetVx == 0.0F);
    assert(GimbalInstance.targetVy == 0.0F);
    assert(GimbalInstance.targetVw == 0.0F);

    RcTask();
    assert(MessageCenterInstance.topicState[MessageId_RcToGimbal].valid == true);
    assert(MessageCenterInstance.topicState[MessageId_RcToGimbal].sequence == 1U);
    assert(MessageCenterInstance.data.rcToGimbal.vx == 1.0F);
    assert(MessageCenterInstance.data.rcToGimbal.vy == -0.5F);
    assert(MessageCenterInstance.data.rcToGimbal.vw == 0.25F);

    GimbalTask();
    assert(GimbalInstance.rcCommand.vx == 1.0F);
    assert(GimbalInstance.rcCommand.vy == -0.5F);
    assert(GimbalInstance.rcCommand.vw == 0.25F);
    assert(GimbalInstance.targetVx == 1.0F);
    assert(GimbalInstance.targetVy == -0.5F);
    assert(GimbalInstance.targetVw == 0.25F);

    return 0;
}
```

- [ ] **Step 2: Compile the module test and verify RED**

Run:

```powershell
gcc -std=c11 -Wall -Wextra -Werror -pedantic -I. tests/test_modules.c message.c -o D:\TMP\message_modules_test.exe
```

Expected: compilation fails because `rc.c` and `gimbal.c` do not exist.

- [ ] **Step 3: Implement the RC publisher**

Create `rc.c`:

```c
#include "message.h"

#include <stdbool.h>

/** @brief RC 模块内部状态。 */
typedef struct
{
    float vx;
    float vy;
    float vw;
} Rc;

/** @brief RC 模块私有实例。 */
static Rc RcInstance;

/** @brief 初始化 RC 演示输入。 */
void RcInit(void)
{
    RcInstance.vx = 1.0F;
    RcInstance.vy = -0.5F;
    RcInstance.vw = 0.25F;
}

/** @brief 将 RC 当前输入发布到消息中心。 */
void RcTask(void)
{
    RcToGimbalMessage rcMessage;
    bool publishSuccess = false;

    rcMessage.vx = RcInstance.vx;
    rcMessage.vy = RcInstance.vy;
    rcMessage.vw = RcInstance.vw;

    if (MessageCenterInstance.Pub.Publish != NULL)
    {
        publishSuccess = MessageCenterInstance.Pub.Publish(
            &MessageCenterInstance,
            MessageId_RcToGimbal,
            &rcMessage,
            sizeof(rcMessage));
    }

    if (!publishSuccess)
    {
        /* 可在此处处理发布失败。 */
    }
}
```

- [ ] **Step 4: Implement the Gimbal subscriber**

Create `gimbal.c`:

```c
#include "message.h"

#include <stdbool.h>
#include <string.h>

/** @brief Gimbal 模块内部状态。 */
typedef struct
{
    RcToGimbalMessage rcCommand;
    float targetVx;
    float targetVy;
    float targetVw;
} Gimbal;

/** @brief Gimbal 模块私有实例。 */
static Gimbal GimbalInstance;

/** @brief 初始化 Gimbal 内部状态。 */
void GimbalInit(void)
{
    memset(&GimbalInstance, 0, sizeof(GimbalInstance));
}

/** @brief 获取 RC 最新命令并更新 Gimbal 目标值。 */
void GimbalTask(void)
{
    bool getSuccess = false;

    if (MessageCenterInstance.Sub.Get != NULL)
    {
        getSuccess = MessageCenterInstance.Sub.Get(
            &MessageCenterInstance,
            MessageId_RcToGimbal,
            &GimbalInstance.rcCommand,
            sizeof(GimbalInstance.rcCommand));
    }

    if (getSuccess)
    {
        GimbalInstance.targetVx = GimbalInstance.rcCommand.vx;
        GimbalInstance.targetVy = GimbalInstance.rcCommand.vy;
        GimbalInstance.targetVw = GimbalInstance.rcCommand.vw;
    }
}
```

- [ ] **Step 5: Compile and run the module test to verify GREEN**

Run:

```powershell
gcc -std=c11 -Wall -Wextra -Werror -pedantic -I. tests/test_modules.c message.c -o D:\TMP\message_modules_test.exe
D:\TMP\message_modules_test.exe
```

Expected: compilation and execution both exit with code `0` and no warnings.

- [ ] **Step 6: Commit the module flow**

```powershell
git add rc.c gimbal.c tests/test_modules.c
git commit -m "feat: add RC and Gimbal message flow"
```

---

### Task 3: Bare-metal-style demo entry point and full verification

**Files:**
- Create: `main.c`
- Verify: `message.h`
- Verify: `message.c`
- Verify: `rc.c`
- Verify: `gimbal.c`
- Verify: `tests/test_message.c`
- Verify: `tests/test_modules.c`

**Interfaces:**
- Consumes: all four module functions produced by Task 2 and `MessageCenterInit` from Task 1.
- Produces: a linkable standard-C application with a bare-metal-style infinite loop.

- [ ] **Step 1: Verify the full application build is RED without `main.c`**

Run:

```powershell
gcc -std=c11 -Wall -Wextra -Werror -pedantic message.c rc.c gimbal.c main.c -o D:\TMP\message_demo.exe
```

Expected: compilation fails because `main.c` does not exist.

- [ ] **Step 2: Implement the demo entry point**

Create `main.c`:

```c
#include "message.h"

/** @brief 初始化 RC 模块。 */
void RcInit(void);

/** @brief 执行一次 RC 主循环任务。 */
void RcTask(void);

/** @brief 初始化 Gimbal 模块。 */
void GimbalInit(void);

/** @brief 执行一次 Gimbal 主循环任务。 */
void GimbalTask(void);

/** @brief 裸机 Demo 入口。 */
int main(void)
{
    /* HAL_Init(); */
    /* SystemClock_Config(); */

    MessageCenterInit(&MessageCenterInstance);
    RcInit();
    GimbalInit();

    while (1)
    {
        RcTask();
        GimbalTask();
    }
}
```

- [ ] **Step 3: Compile the complete demo to verify GREEN**

Run:

```powershell
gcc -std=c11 -Wall -Wextra -Werror -pedantic message.c rc.c gimbal.c main.c -o D:\TMP\message_demo.exe
```

Expected: compilation and linking exit with code `0` and no warnings. Do not run the infinite-loop executable.

- [ ] **Step 4: Run both regression suites freshly**

Run:

```powershell
gcc -std=c11 -Wall -Wextra -Werror -pedantic -I. tests/test_message.c message.c -o D:\TMP\message_center_test.exe
D:\TMP\message_center_test.exe
gcc -std=c11 -Wall -Wextra -Werror -pedantic -I. tests/test_modules.c message.c -o D:\TMP\message_modules_test.exe
D:\TMP\message_modules_test.exe
```

Expected: all four commands exit with code `0` and no warnings or assertion failures.

- [ ] **Step 5: Review every requirement and inspect the diff**

Run:

```powershell
git diff --check
git status --short
```

Expected: `git diff --check` produces no output; status lists only the intended source, test, and plan changes.

- [ ] **Step 6: Commit the completed demo and plan**

```powershell
git add main.c docs/superpowers/plans/2026-07-21-message-center.md
git commit -m "feat: complete bare-metal message center demo"
```
