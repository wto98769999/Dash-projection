#include "bufferclean.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("BufferCleanAlgorithm");
NS_OBJECT_ENSURE_REGISTERED(BufferCleanAlgorithm);

BufferCleanAlgorithm::BufferCleanAlgorithm(const videoData &videoData,
                                           const playbackData &playbackData,
                                           const bufferData &bufferData,
                                           const throughputData &throughput)
    : BufferAlgorithm(videoData, playbackData, bufferData, throughput),
      m_bandwidthAlgoIndex(1),
      m_bufferTargetNumber(m_videoData.segmentDuration * 5),
      m_highestRepIndex(videoData.averageBitrate[0].size() - 1) {
  NS_LOG_INFO(this);
  NS_ASSERT_MSG(m_highestRepIndex >= 0,
                "The highest quality representation index should be >= 0");
}

bufferAlgoReply BufferCleanAlgorithm::BufferAlgo(
    const int64_t segmentCounter, const int64_t clientId,
    int64_t extraParameter,   // bufferCleanNumber
    int64_t extraParameter2)  // bufferTargeNumber
{
  bufferAlgoReply answer;
  answer.bufferAlgoIndex = 1;
  const int64_t timeNow = Simulator::Now().GetMicroSeconds();
  answer.decisionTime = timeNow;
  answer.bufferCleanNumber = extraParameter;
  answer.bufferTargetNumber = extraParameter2;
  return answer;
}

}  // namespace ns3