#ifndef MEDIA_COMMON_UTILS_H_
#define MEDIA_COMMON_UTILS_H_
#include <Windows.h>
#include <stdint.h>

void SleepAccurate(int ms);

//void InitBitMapInfoHeader(int width, int height, bool top2bottom, int bit_count, BITMAPINFOHEADER *bih);
//
//void InitBitMapInfo(int width, int height, bool top2bottom, int bit_count, BITMAPINFO *bmi);

int64_t TimeMilliseconds();

int64_t TimeMicroseconds();

int64_t TimeNanoseconds();

#endif // MEDIA_COMMON_UTILS_H_
