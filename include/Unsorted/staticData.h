#ifndef STATIC_DATA_H
#define STATIC_DATA_H

#include <version.h>

#if VERSION == VERSION_JPN
extern u8 staticdata_arc[];
#elif VERSION == VERSION_USA
extern u8 staticdata_usa_arc[];
#endif

#endif
