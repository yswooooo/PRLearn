#include "message.h"

#include <stdbool.h>
#include <string.h>

/** @brief Gimbal 模块内部状态。 */
typedef struct
{
    RcToGimbalMessage rcCommand; /**< 最近一次成功获取的 RC 命令。 */

    float targetVx; /**< Gimbal 的 X 方向目标速度。 */
    float targetVy; /**< Gimbal 的 Y 方向目标速度。 */
    float targetVw; /**< Gimbal 的旋转目标速度。 */
} Gimbal;

/** @brief Gimbal 模块私有实例。 */
static Gimbal GimbalInstance;

/**
 * @brief 初始化 Gimbal 内部状态。
 */
void GimbalInit(void)
{
    memset(&GimbalInstance, 0, sizeof(GimbalInstance));
}

/**
 * @brief 获取 RC 最新命令并更新 Gimbal 目标值。
 */
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
