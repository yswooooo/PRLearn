#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief RC 发送给 Gimbal 的专用通信数据。
 */
typedef struct
{
    float vx; /**< X 方向速度命令。 */
    float vy; /**< Y 方向速度命令。 */
    float vw; /**< 旋转速度命令。 */
} RcToGimbalMessage;

/**
 * @brief 消息通道标识。
 * @note 枚举值可直接用作 MessageCenter::topicState 的数组下标。
 */
typedef enum
{
    MessageId_None = 0,
    MessageId_RcToGimbal,
    MessageId_Count
} MessageId;

struct MessageCenter;

/**
 * @brief 消息发布函数类型。
 *
 * @param self 消息中心对象。
 * @param messageId 消息通道标识。
 * @param inputData 待发布数据。
 * @param dataSize 待发布数据长度。
 * @return 发布成功返回 true，否则返回 false。
 */
typedef bool (*MessagePublishFunction)(
    struct MessageCenter *self,
    MessageId messageId,
    const void *inputData,
    size_t dataSize);

/**
 * @brief 消息获取函数类型。
 *
 * @param self 消息中心对象。
 * @param messageId 消息通道标识。
 * @param outputData 接收消息的输出区域。
 * @param dataSize 输出区域期望的数据长度。
 * @return 获取成功返回 true，否则返回 false。
 */
typedef bool (*MessageGetFunction)(
    struct MessageCenter *self,
    MessageId messageId,
    void *outputData,
    size_t dataSize);

/** @brief 消息发布接口。 */
typedef struct
{
    MessagePublishFunction Publish; /**< 发布一条消息。 */
} MessagePublisher;

/** @brief 消息订阅接口。 */
typedef struct
{
    MessageGetFunction Get; /**< 获取指定通道的最新消息。 */
} MessageSubscriber;

/** @brief 单个消息通道的状态。 */
typedef struct
{
    bool valid;       /**< 是否至少成功发布过一次。 */
    uint32_t sequence; /**< 成功发布的累计次数。 */
} MessageTopicState;

/** @brief 消息中心保存的全部最新消息数据。 */
typedef struct
{
    RcToGimbalMessage rcToGimbal; /**< RC 到 Gimbal 的最新消息。 */
} MessageData;

/**
 * @brief 最新值消息中心。
 *
 * 每个消息 ID 只保存一份最新数据，读取后不会删除数据。
 *
 * @note 当前实现仅适用于裸机主循环上下文，不支持中断与主循环并发访问。
 */
typedef struct MessageCenter
{
    MessagePublisher Pub; /**< 发布接口。 */
    MessageSubscriber Sub; /**< 获取接口。 */

    MessageTopicState topicState[MessageId_Count]; /**< 各通道状态。 */

    MessageData data; /**< 最新消息存储区。 */
} MessageCenter;

/** @brief 全局唯一消息中心实例。 */
extern MessageCenter MessageCenterInstance;

/**
 * @brief 初始化消息中心对象。
 *
 * @param self 待初始化的消息中心；为 NULL 时不执行操作。
 */
void MessageCenterInit(MessageCenter *self);

#endif
