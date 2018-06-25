#include "bandwidth-avgintime.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("BandwidthAvgInTimeAlgorithm");
NS_OBJECT_ENSURE_REGISTERED(BandwidthAvgInTimeAlgorithm);

BandwidthAvgInTimeAlgorithm::BandwidthAvgInTimeAlgorithm(
    const videoData &videoData, const playbackData &playbackData,
    const bufferData &bufferData, const throughputData &throughput)
    : BandwidthAlgorithm(videoData, playbackData, bufferData, throughput),
      // m_deltaTime(m_videoData.segmentDuration * 3),
      m_deltaTime_1(10000000),
      m_deltaTime_2(30000000),
      m_lastBandwidthEstimate(0),
      m_highestRepIndex(videoData.averageBitrate[0].size() - 1) {
  NS_LOG_INFO(this);
  NS_ASSERT_MSG(m_highestRepIndex >= 0,
                "The highest quality representation index should be >= 0");
}

bandwidthAlgoReply BandwidthAvgInTimeAlgorithm::BandwidthAlgo(
    const int64_t segmentCounter, const int64_t clientId) {
  bandwidthAlgoReply answer;
  answer.bandwidthAlgoIndex = 1;
  const int64_t timeNow = Simulator::Now().GetMicroSeconds();
  answer.decisionTime = timeNow;
  if (timeNow <= 10000000) {
    answer.bandwidthEstimate = (int64_t)AverageBandwidth(
        timeNow - m_deltaTime_1, timeNow, answer.decisionCase);
  } else {
    answer.bandwidthEstimate =
        0.5 * (int64_t)AverageBandwidth(timeNow - m_deltaTime_1, timeNow,
                                        answer.decisionCase) +
        0.5 * (int64_t)AverageBandwidth(timeNow - m_deltaTime_2,
                                        timeNow - m_deltaTime_1,
                                        answer.decisionCase);
  }

  return answer;
}
double BandwidthAvgInTimeAlgorithm::AverageBandwidth(int64_t t_1, int64_t t_2,
                                                     int64_t &decisioncase) {
  if (t_1 < 0) {
    t_1 = 0;
  }
  if (m_throughput.transmissionEnd.size() < 1) {
    decisioncase = 0;
    return m_lastBandwidthEstimate;
  }

  int32_t indexStart = 0;
  int32_t indexEnd = m_throughput.transmissionEnd.size() - 1;
  for (int32_t i = 0; i <= int32_t(m_throughput.transmissionStart.size() - 1);
       i++) {
    if (m_throughput.transmissionEnd.at(i) < t_1)
      continue;
    else {
      indexStart = i;
      break;
    }
  }
  for (int32_t j = int32_t(m_throughput.transmissionEnd.size() - 1); j >= 0;
       j--) {
    if (m_throughput.transmissionStart.at(j) > t_2)
      continue;
    else {
      indexEnd = j;
      break;
    }
  }
  NS_ASSERT_MSG(indexEnd >= indexStart, "indexEnd should be >= indexStart");
  double sumThroughput = 0.0;
  double transmissionTime = 0.0;
  while (indexStart <= indexEnd) {
    sumThroughput +=
        8 * m_videoData.segmentSize.at(m_videoData.userInfo.at(indexStart))
                .at(m_videoData.repIndex.at(indexStart))
                .at(indexStart);
    transmissionTime +=
        (double)(m_throughput.transmissionEnd.at(indexStart) -
                 m_throughput.transmissionStart.at(indexStart)) /
        1000000.0;
    indexStart++;
  }
  decisioncase = 1;
  m_lastBandwidthEstimate = (sumThroughput / (double)transmissionTime);
  return m_lastBandwidthEstimate;
}

}  // namespace ns3