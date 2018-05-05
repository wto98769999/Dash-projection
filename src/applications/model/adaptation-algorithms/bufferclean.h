#ifndef BUFFERCLEAN_ALGORITHM_H
#define BUFFERCLEAN_ALGORITHM_H
#include "tcp-stream-buffer-algorithm.h"

namespace ns3 {

class BufferCleanAlgorithm : public BufferAlgorithm {
 public:
  BufferCleanAlgorithm(const videoData &videoData,
                       const playbackData &playbackData,
                       const bufferData &bufferData,
                       const throughputData &throughput);

  bufferAlgoReply BufferAlgo(const int64_t segmentCounter,
                             const int64_t clientId, int64_t extraParameter,
                             int64_t extraParameter2);

 private:
  const int64_t
      m_bandwidthAlgoIndex;  // bufferClean = 1 bufferAdaptive = 2 default = 0
  const int64_t
      m_bufferTargetNumber;  // default bufferTargetNumber = 10s (10000000)
  const int64_t m_highestRepIndex;
};

}  // namespace ns3
#endif /* BUFFERCLEAN_ALGORITHM_H */