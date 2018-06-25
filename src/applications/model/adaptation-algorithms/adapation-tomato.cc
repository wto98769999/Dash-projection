#include "adapation-tomato.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("TomatoAlgorithm");
NS_OBJECT_ENSURE_REGISTERED(TomatoAlgorithm);

TomatoAlgorithm::TomatoAlgorithm(const videoData &videoData,
                                 const playbackData &playbackData,
                                 const bufferData &bufferData,
                                 const throughputData &throughput)
    : AdaptationAlgorithm(videoData, playbackData, bufferData, throughput),
      m_lastRepIndex(0),  // last Bitrate Level
      /*
      m_targetBuffer(m_videoData.segmentDuration * 5),  // 5s
      m_bufferMin(m_videoData.segmentDuration * 2),     // 2s
      */
      m_targetBuffer(15000000),  // 15s(30000000)
      m_bufferMin(10000000),     // 10s
      m_expBuffer(0),  // buffer expection ,cal at the beginning of download the
                       // current seg
      m_multipleTinyDrop(0),  // cal tiny buffer drop
      m_beta(0.0),            // adjust buffer upper bound
                              /*
                              m_bufferUpperbound(m_videoData.segmentDuration * 15),  // 15s
                              */
      m_bufferUpperbound(40000000),
      m_highestRepIndex(videoData.averageBitrate[0].size() - 1) {
  NS_LOG_INFO(this);
  NS_ASSERT_MSG(m_highestRepIndex >= 0,
                "The highest quality representation index should be >= 0");
}

algorithmReply TomatoAlgorithm::GetNextRep(const int64_t segmentCounter,
                                           const int64_t clientId,
                                           int64_t bandwidth) {
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
    double alpha =
        bufferNow > m_targetBuffer ? 1.0 : bufferNow / m_targetBuffer;
    if (bufferNow < m_bufferMin) {
      answer.nextRepIndex = 0;
      answer.decisionCase = 0;
    } else {
      if (bandwidth > 0) {
        int64_t nextHighestIndex = m_highestRepIndex;

        while (nextHighestIndex > 0 &&
               m_videoData.averageBitrate
                       .at(m_videoData.userInfo.at(segmentCounter))
                       .at(nextHighestIndex) > bandwidth * alpha) {
          nextHighestIndex--;
        }
        if (nextHighestIndex > m_lastRepIndex) {
          if (bufferNow > m_expBuffer) {
            if (bufferNow < m_expBuffer + m_videoData.segmentDuration / 2) {
              nextHighestIndex = m_lastRepIndex + 1;
              answer.decisionCase = 1;
            } else
              // nextHighestIndex =nextHighestIndex;
              answer.decisionCase = 2;
          } else {
            nextHighestIndex = m_lastRepIndex;
            answer.decisionCase = 3;
          }
        } else if (nextHighestIndex < m_lastRepIndex) {
          if (bufferNow < m_expBuffer - 3 * m_videoData.segmentDuration) {
            nextHighestIndex = nextHighestIndex;
            m_multipleTinyDrop = 0;
            answer.decisionCase = 4;
          } else if (bufferNow <
                     m_expBuffer - 2 * m_videoData.segmentDuration) {
            nextHighestIndex = std::min(nextHighestIndex, m_lastRepIndex / 2);
            m_multipleTinyDrop = 0;
            answer.decisionCase = 5;
          } else if (bufferNow <
                     m_expBuffer - 1 * m_videoData.segmentDuration) {
            nextHighestIndex = std::max(nextHighestIndex, m_lastRepIndex / 2);
            m_multipleTinyDrop = 0;
            answer.decisionCase = 6;
          } else if (bufferNow <
                     m_expBuffer - m_videoData.segmentDuration / 2) {
            nextHighestIndex = m_lastRepIndex - 1;
            m_multipleTinyDrop = 0;
            answer.decisionCase = 7;
          } else {
            if (m_multipleTinyDrop >= 3) {
              nextHighestIndex = m_lastRepIndex - 1;
              m_multipleTinyDrop = 0;
              answer.decisionCase = 8;
            } else {
              nextHighestIndex = m_lastRepIndex;
              answer.decisionCase = 9;
              if (bufferNow < m_expBuffer) ++m_multipleTinyDrop;
            }
          }
        } else {
          // nextHighestIndex == m_lastRepIndex
          answer.decisionCase = 10;
        }
        answer.nextRepIndex = nextHighestIndex;
      } else {
        answer.nextRepIndex = 0;
        answer.decisionCase = -1;
      }
    }

    double lastbeta = m_beta;  // lastbeta
    m_beta = 1.0 + answer.nextRepIndex /
                       (double)m_highestRepIndex;  // beta base value = 1.0;

    // insure beta is smooth
    if (m_beta < lastbeta) {
      m_beta = lastbeta - 1.0 / (double)m_highestRepIndex;
    }

    // cal next downloaddelay
    if (bufferNow > m_targetBuffer * m_beta &&
        answer.nextRepIndex > m_lastRepIndex && bufferNow > m_expBuffer) {
      answer.nextDownloadDelay = bufferNow - m_targetBuffer * m_beta;
      answer.delayDecisionCase = 1;

      // check whether the delaytime can be used for a hgher repindex
      if (answer.nextRepIndex < m_highestRepIndex && bandwidth > 0 &&
          ((answer.nextRepIndex + 1) == m_lastRepIndex)) {
        int64_t lastSegmentThroughput =
            8 *
            (m_videoData.segmentSize
                 .at(m_videoData.userInfo.at(segmentCounter - 1))
                 .at(m_lastRepIndex)
                 .at(segmentCounter - 1)) *
            1000000 /
            (m_throughput.transmissionEnd.at(segmentCounter - 1) -
             m_throughput.transmissionStart.at(segmentCounter - 1));

        double extraDonwloadTime =
            (m_videoData.averageBitrate
                 .at(m_videoData.userInfo.at(segmentCounter))
                 .at(answer.nextRepIndex + 1) -
             m_videoData.averageBitrate
                 .at(m_videoData.userInfo.at(segmentCounter))
                 .at(answer.nextRepIndex)) /
            //(lastSegmentThroughput * alpha / 1000);
            (lastSegmentThroughput / 1000);

        if (answer.nextDownloadDelay > extraDonwloadTime) {
          answer.nextRepIndex = answer.nextRepIndex + 1;
          answer.nextDownloadDelay =
              answer.nextDownloadDelay > extraDonwloadTime;
          answer.decisionCase = 10;
          answer.delayDecisionCase = 2;
        }
      }
    }
  } else {
    answer.decisionCase = 0;
    answer.nextRepIndex = m_lastRepIndex;
    answer.nextDownloadDelay = 0;
    answer.decisionCase = 0;
  }
  answer.estimateTh = bandwidth;
  m_lastRepIndex = answer.nextRepIndex;
  m_expBuffer =
      answer.nextDownloadDelay > 0 ? m_targetBuffer * m_beta : bufferNow;

  // add for xhinaxobile
  if (m_expBuffer > m_bufferUpperbound - m_videoData.segmentDuration) {
    answer.nextDownloadDelay +=
        m_expBuffer - (m_bufferUpperbound - m_videoData.segmentDuration);
  }
  //

  return answer;
}  // namespace ns3

}  // namespace ns3