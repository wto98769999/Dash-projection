#ifndef BUFFER_ALGORITHM_H
#define BUFFER_ALGORITHM_H
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include "ns3/application.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "tcp-stream-interface.h"

namespace ns3 {
// todo
class BufferAlgorithm : public Object {
 public:
  BufferAlgorithm(const videoData &videoData, const playbackData &playbackData,
                  const bufferData &bufferData,
                  const throughputData &throughput);

  virtual bufferAlgoReply BufferAlgo(const int64_t segmentCounter,
                                     const int64_t clientId,
                                     int64_t extraParameter,
                                     int64_t extraParameter2) = 0;

 protected:
  const videoData &m_videoData;
  const bufferData &m_bufferData;
  const throughputData &m_throughput;
  const playbackData &m_playbackData;
};

}  // namespace ns3
#endif /* BUFFER_ALGORITHM_H */