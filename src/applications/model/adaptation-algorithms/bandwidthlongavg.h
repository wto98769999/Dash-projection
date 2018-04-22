#ifndef BANDWIDTHLONGAVG_ALGORITHM_H
#define BANDWIDTHLONGAVG_ALGORITHM_H
#include "tcp-stream-bandwidth-algorithm.h"

namespace ns3 {

class BandwidthLongAvgAlgorithm : public BandwidthAlgorithm {
public:
  BandwidthLongAvgAlgorithm(const videoData &videoData,
                            const playbackData &playbackData,
                            const bufferData &bufferData,
                            const throughputData &throughput);

  bandwidthAlgoReply BandwidthAlgo(const int64_t segmentCounter,
                                   const int64_t clientId,
                                   int64_t extraParameter,
                                   int64_t extraParameter2);

private:
  const int64_t
      m_bandwidthAlgoIndex; // bandwidthAvgInTime(Average Of DeltaTime) = 1,
                            // bandwidthCrosslayer(get From PHYlayer) = 2,
                            // bandwidthLongAvg(long-Term Average) = 3,
                            // bandwdithAvgInChunk(short-Term Average) = 4,
                            // bandwidthHarmonic = 5
  double m_lastBandwidthEstimate; // Last bandwidthEstimate Value
  int64_t m_lastRepIndex;
  const int64_t m_highestRepIndex;
};

} // namespace ns3
#endif /* BANDWIDTHLONGAVG_ALGORITHM_H */