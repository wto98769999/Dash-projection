#include "tcp-stream-buffer-algorithm.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("BufferAlgorithm");
NS_OBJECT_ENSURE_REGISTERED(BufferAlgorithm);

BufferAlgorithm::BufferAlgorithm(const videoData &videoData,
                                 const playbackData &playbackData,
                                 const bufferData &bufferData,
                                 const throughputData &throughput)
    : m_videoData(videoData),
      m_bufferData(bufferData),
      m_throughput(throughput),
      m_playbackData(playbackData) {}

}  // namespace ns3