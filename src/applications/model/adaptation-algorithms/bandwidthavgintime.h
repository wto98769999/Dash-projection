#ifndef BANDWIDTHAVGINTIME_ALGORITHM_H
#define BANDWIDTHAVGINTIME_ALGORITHM_H
#include "tcp-stream-bandwidth-algorithm.h"

namespace ns3 {

class BandwidthAvgInTimeAlgorithm : public BandwidthAlgorithm {
public:
  BandwidthAvgInTimeAlgorithm(const videoData &videoData,
                              const playbackData &playbackData,
                              const bufferData &bufferData,
                              const throughputData &throughput);

  bandwidthAlgoReply BandwidthAlgo(const int64_t segmentCounter,
                                   const int64_t clientId,
                                   int64_t extraParameter,   // reservation
                                   int64_t extraParameter2); // reservation
private:
  double AverageBandwidth(int64_t t1, int64_t t2, int64_t &decisioncase);

  const int64_t
      m_bandwidthAlgoIndex; // bandwidthAvgInTime(Average Of DeltaTime) = 1,
                            // bandwidthCrosslayer(get From PHYlayer) = 2,
                            // bandwidthLongAvg(long-Term Average) = 3,
                            // bandwdithAvgInChunk(short-Term Average) = 4,
                            // bandwidthHarmonic = 5
  int64_t m_deltaTime; // estimate bandwith in [m_t1, m_t2].length = m_time,
                       // default = 10s
  double m_lastBandwidthEstimate; // Last bandwidthEstimate Value
  const int64_t m_highestRepIndex;
};

} // namespace ns3
#endif /* BANDWIDTHAVGINTIME_ALGORITHM_H */