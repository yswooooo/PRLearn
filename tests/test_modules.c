#include "message.h"

#include <assert.h>

/*
 * 本测试将模块源码放入同一翻译单元，以便验证模块私有状态。
 * 产品构建仍然分别编译 rc.c 和 gimbal.c。
 */
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
