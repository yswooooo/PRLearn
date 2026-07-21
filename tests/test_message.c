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
