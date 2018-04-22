#include "adapationtomato2.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Tomato2Algorithm");
NS_OBJECT_ENSURE_REGISTERED(Tomato2Algorithm);

Tomato2Algorithm::Tomato2Algorithm(const videoData &videoData,
                                   const playbackData &playbackData,
                                   const bufferData &bufferData,
                                   const throughputData &throughput)
    : AdaptationAlgorithm(videoData, playbackData, bufferData, throughput),
      m_lastRepIndex(0), m_targetBuffer(m_videoData.segmentDuration * 5),
      m_deltaBuffer(m_videoData.segmentDuration * 1),
      m_bufferMin(m_videoData.segmentDuration * 2), m_lastBuffer(0),
      m_highestRepIndex(videoData.averageBitrate[0].size() - 1) {
  NS_LOG_INFO(this);
  NS_ASSERT_MSG(m_highestRepIndex >= 0,
                "The highest quality representation index should be >= 0");
}

algorithmReply Tomato2Algorithm::GetNextRep(const int64_t segmentCounter,
                                            const int64_t clientId,
                                            int64_t extraParameter,
                                            int64_t extraParameter2) {
  algorithmReply answer;
  answer.decisionCase = 0;
  answer.delayDecisionCase = 0;
  answer.nextDownloadDelay = 0;
  const int64_t timeNow = Simulator::Now().GetMicroSeconds();
  answer.decisionTime = timeNow;
  int64_t bufferNow = 0;
  if (segmentCounter != 0) {
    bufferNow = m_bufferData.bufferLevelNew.back() -
                (timeNow - m_throughput.transmissionEnd.back());
    if (bufferNow <= m_bufferMin) {
      answer.nextRepIndex = 0;
      answer.decisionCase = 1;
    } else {
      if (extraParameter > 0) {
        int64_t nextHighestIndex = m_highestRepIndex;
        double alpha =
            (bufferNow > m_targetBuffer * 0.90)
                ? 0.95
                : ((bufferNow > m_targetBuffer * 0.70)
                       ? 0.85
                       : ((bufferNow > m_targetBuffer * 0.5) ? 0.75 : 0.65));
        while (nextHighestIndex > 0 &&
               m_videoData.averageBitrate
                       .at(m_videoData.userInfo.at(segmentCounter))
                       .at(nextHighestIndex) > extraParameter * alpha) {
          nextHighestIndex--;
        }
        if (nextHighestIndex > m_lastRepIndex) {
          if (bufferNow >= m_lastBuffer)
            nextHighestIndex = m_lastRepIndex + 1;
          else
            nextHighestIndex = m_lastRepIndex;
          answer.decisionCase = 3;
        } else if (nextHighestIndex < m_lastRepIndex) {
          if (bufferNow < m_lastBuffer - m_videoData.segmentDuration)
            nextHighestIndex = std::min(m_lastRepIndex - 1, m_lastRepIndex / 2);
          else if (bufferNow > m_targetBuffer)
            nextHighestIndex = m_lastRepIndex;
          else
            nextHighestIndex =
                (bufferNow > m_targetBuffer * 0.70)
                    ? m_lastRepIndex - 1
                    : std::min(m_lastRepIndex - 1, m_lastRepIndex / 2);
          answer.decisionCase = 4;
        } else {
          answer.decisionCase = 5;
        }
        answer.nextRepIndex = nextHighestIndex;
      } else {
        answer.nextRepIndex = 0;
        answer.decisionCase = 2;
      }
    }
    double beta = 1.0 + (double)answer.nextRepIndex / (double)m_highestRepIndex;
    if (bufferNow > m_targetBuffer * beta) {
      int64_t lowerBound = m_targetBuffer * beta - m_deltaBuffer / 2;
      int64_t upperBound = m_targetBuffer * beta + m_deltaBuffer / 2;
      int64_t randBuf =
          (int64_t)lowerBound + (std::rand() % (upperBound - (lowerBound) + 1));
      if (bufferNow > randBuf) {
        answer.nextDownloadDelay = (int64_t)(bufferNow - randBuf);
        answer.delayDecisionCase = 1;
      }
    } else {
      answer.nextDownloadDelay = 0;
      answer.decisionCase = 0;
    }
  } else {
    answer.decisionCase = 0;
    answer.nextRepIndex = m_lastRepIndex;
    answer.nextDownloadDelay = 0;
    answer.decisionCase = 0;
  }
  answer.estimateTh = extraParameter;
  m_lastRepIndex = answer.nextRepIndex;

  return answer;
}

} // namespace ns3