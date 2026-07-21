#include "message.h"

#include <string.h>

/**
 * @brief 发布消息的内部实现。
 */
static bool MessageCenterPublishImpl(
    MessageCenter *self,
    MessageId messageId,
    const void *inputData,
    size_t dataSize);

/**
 * @brief 获取消息的内部实现。
 */
static bool MessageCenterGetImpl(
    MessageCenter *self,
    MessageId messageId,
    void *outputData,
    size_t dataSize);

/** @brief 全局唯一消息中心实例定义。 */
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
