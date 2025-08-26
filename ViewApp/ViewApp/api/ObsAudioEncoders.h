
#include <map>
#include <obs.hpp>

const std::map<int, const char*> &GetAACEncoderBitrateMap();
const char *GetAACEncoderForBitrate(int bitrate);
int FindClosestAvailableAACBitrate(int bitrate);

