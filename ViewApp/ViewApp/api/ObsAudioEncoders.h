
#include <map>
#include <obs.hpp>

const std::map<int, const char*> &GetAACEncoderBitrateMap();
const char *GetAACEncoderForBitrate(int bitrate);
//找到bitrate 最近的码率
int FindClosestAvailableAACBitrate(int bitrate);

