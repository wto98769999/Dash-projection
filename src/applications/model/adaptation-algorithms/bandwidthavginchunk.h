#ifndef BANDWIDTHAVGINCHUNK_ALGORITHM_H
#define BANDWIDTHAVGINCHUNK_ALGORITHM_H
#include "tcp-stream-bandwidth-algorithm.h"

namespace ns3 {

class BandwidthAvgInChunkAlgorithm : public BandwidthAlgorithm {
public:
  BandwidthAvgInChunkAlgorithm(const videoData &videoData,
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
  std::vector<double> m_lastDownloadRate;
  const int64_t m_windowSize;
  const int64_t m_highestRepIndex;
};

} // namespace ns3
#endif /* BANDWIDTHAVGINCHUNK_ALGORITHM_H */