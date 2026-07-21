#include "message.h"

#include <stdbool.h>

/** @brief RC 模块内部状态。 */
typedef struct
{
    float vx; /**< X 方向速度输入。 */
    float vy; /**< Y 方向速度输入。 */
    float vw; /**< 旋转速度输入。 */
} Rc;

/** @brief RC 模块私有实例。 */
static Rc RcInstance;

/**
 * @brief 初始化 RC 演示输入。
 */
void RcInit(void)
{
    RcInstance.vx = 1.0F;
    RcInstance.vy = -0.5F;
    RcInstance.vw = 0.25F;
}

/**
 * @brief 将 RC 当前输入发布到消息中心。
 */
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
