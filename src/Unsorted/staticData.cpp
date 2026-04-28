//TODO: Is this by itself or is it in the same tu with functions that use it?
#include "Unsorted/staticData.h"

//NOTE: Files are created during setup
#if VERSION == VERSION_JPN
#include "staticdata_arc.inc"
#elif VERSION == VERSION_USA
#include "staticdata_usa_arc.inc"
#endif
