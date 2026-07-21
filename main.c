#include "message.h"

/** @brief 初始化 RC 模块。 */
void RcInit(void);

/** @brief 执行一次 RC 主循环任务。 */
void RcTask(void);

/** @brief 初始化 Gimbal 模块。 */
void GimbalInit(void);

/** @brief 执行一次 Gimbal 主循环任务。 */
void GimbalTask(void);

/**
 * @brief 裸机 MessageCenter Demo 入口。
 *
 * @return 裸机主循环不会返回。
 */
int main(void)
{
    /* HAL_Init(); */
    /* SystemClock_Config(); */

    MessageCenterInit(&MessageCenterInstance);
    RcInit();
    GimbalInit();
    //创建一个PR尝试 
    //创建二个PR尝试 //创建三个PR尝试
    while (1)
    {
        RcTask();
        GimbalTask();
    }
}
