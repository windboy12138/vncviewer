#include "common_utils.h"
#include <timeapi.h>
#pragma comment(lib, "winmm.lib") // for timer

namespace
{
    const DWORD kNormalTerminationExitCode = 0;
    const DWORD kDebuggerInactiveExitCode = 0xC0000354;
    const DWORD kKeyboardInterruptExitCode = 0xC000013A;
    const DWORD kDebuggerTerminatedExitCode = 0x40010004;
    const DWORD kProcessKilledExitCode = 1;

    const int64_t kNumMillisecsPerSec = INT64_C(1000);
    const int64_t kNumMicrosecsPerSec = INT64_C(1000000);
    const int64_t kNumNanosecsPerSec = INT64_C(1000000000);

    const int64_t kNumMicrosecsPerMillisec = kNumMicrosecsPerSec / kNumMillisecsPerSec;
    const int64_t kNumNanosecsPerMillisec = kNumNanosecsPerSec / kNumMillisecsPerSec;
    const int64_t kNumNanosecsPerMicrosec = kNumNanosecsPerSec / kNumMicrosecsPerSec;

} // namespace

void SleepAccurate(int ms)
{
    int64_t end = TimeMicroseconds() + ms * 1000LL;
    for (int64_t now = TimeMicroseconds(); now < end; now = TimeMicroseconds())
        ::Sleep((end - now) / 1000LL);
}

//void InitBitMapInfoHeader(int width, int height, bool top2bottom, int bit_count, BITMAPINFOHEADER *bih)
//{
//    memset(bih, 0, sizeof(BITMAPINFOHEADER));
//    bih->biSize = sizeof(BITMAPINFOHEADER);
//    bih->biWidth = width;
//    bih->biHeight = top2bottom ? -height : height;
//    bih->biPlanes = 1;
//    bih->biBitCount = bit_count;
//    bih->biCompression = BI_RGB;
//    bih->biSizeImage = width * height * 4;
//}
//
//void InitBitMapInfo(int width, int height, bool top2bottom, int bit_count, BITMAPINFO *bmi)
//{
//    InitBitMapInfoHeader(width, height, top2bottom, bit_count, &bmi->bmiHeader);
//    memset(bmi->bmiColors, 0, sizeof(RGBQUAD));
//}

int64_t SystemTimeNanos()
{
    int64_t ticks = 0LL;
    static volatile LONG last_timegettime = 0;
    static volatile int64_t num_wrap_timegettime = 0;
    volatile LONG* last_timegettime_ptr = &last_timegettime;
    DWORD now = timeGetTime();
    DWORD old = InterlockedExchange(last_timegettime_ptr, now);
    if (now < old)
    {
        if (old > 0xf0000000 && now < 0x0fffffff)
        {
            num_wrap_timegettime++;
        }
    }
    ticks = now + (num_wrap_timegettime << 32);
    ticks = ticks * kNumNanosecsPerMillisec;
    return ticks;
}

int64_t TimeMilliseconds()
{
    return TimeNanoseconds() / kNumNanosecsPerMillisec;
}

int64_t TimeMicroseconds()
{
    return TimeNanoseconds() / kNumNanosecsPerMicrosec;
}

int64_t TimeNanoseconds()
{
    return SystemTimeNanos();
}