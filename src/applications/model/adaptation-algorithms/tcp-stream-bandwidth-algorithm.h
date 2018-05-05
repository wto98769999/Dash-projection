#ifndef BANDWIDTH_ALGORITHM_H
#define BANDWIDTH_ALGORITHM_H
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

class BandwidthAlgorithm : public Object {
 public:
  BandwidthAlgorithm(const videoData &videoData,
                     const playbackData &playbackData,
                     const bufferData &bufferData,
                     const throughputData &throughput);

  virtual bandwidthAlgoReply BandwidthAlgo(const int64_t segmentCounter,
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
#endif /* BANDWIDTH_ALGORITHM_H */