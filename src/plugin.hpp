#include <rack.hpp>

using namespace rack;

extern Plugin *pluginInstance;

extern Model *modelDelay;
extern Model *modelMultitap;

MenuItem *createRangeItem(std::string label, float *gain, float *offset);
