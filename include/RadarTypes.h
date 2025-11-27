#ifndef RadarTypes_H
#define RadarTypes_H
#include <cstdint>
#include <vector>
#include <deque>
#include <iostream>
#include <cmath>

struct Vec2
{
    float x;
    float y;
    Vec2() : x(0), y(0) {}
    Vec2(float _x, float _y) : x(_x), y(_y) {}
};

struct Vec4
{
    float r;
    float g;
    float b;
    float a;
    Vec4() : r(0), g(0), b(0), a(0) {}
    Vec4(float _r, float _g, float _b, float _a) : r(_r), g(_g), b(_b), a(_a) {}
};

struct RadarVertex
{
    Vec2 position;
    Vec4 color;
};

struct RadarVideoSweep
{
    uint16_t sequence;
    float azimuth;
    std::vector<uint8_t> intensities;
};

struct RadarState
{
    std::deque<RadarVideoSweep> history;
    float lastAzimuth;
    int lastSeq;
    int skipCount = 0;
    float tail = 0.0f;
    size_t MAX_SKIP = 180;
    float MAX_TAIL = 300.0f;
};

inline bool seqNewer(uint16_t current, int previous)
{
    if (previous < 0)
        return true;
    return static_cast<int16_t>(current - previous) > 0;
}

inline float normalizeAz(float azimuth)
{
    float res = fmodf(azimuth, 360.0f);
    if (res < 0.0f)
        res += 360.0f;
    return res;
}

inline float normalizeAzDiff(float nextAz, float prevAz)
{
    float d = nextAz - prevAz;
    return normalizeAz(d);
}

inline bool isStrictlyNewer(int prevSequence, float prevAzimuth, int nextSequence, float nextAzimuth)
{
    if (!seqNewer(nextSequence, prevSequence))
        return false;

    float azDiff = normalizeAzDiff(nextAzimuth, prevAzimuth);
    return (azDiff > 0.0f);
}

inline void processSkip(RadarState &state, int currentSequence, float currentAzimuth)
{
    state.skipCount++;
    // std::cout << "skipped azimuth sequence: " << state.skipCount << " sequence: " << currentSequence << " last sequence accepted: " << state.lastSeq << std::endl;

    if (state.skipCount >= state.MAX_SKIP)
    {
        state.lastSeq = currentSequence;
        state.lastAzimuth = currentAzimuth;
        state.skipCount = 0;
        std::cout << ">> reset sequence: " << currentSequence << " last sequence: " << state.lastSeq << std::endl;
    }
}
#endif