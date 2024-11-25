#pragma once

#include "directxtk12/SimpleMath.h"
using namespace DirectX::SimpleMath;

static constexpr float Pi = 3.14159265f;

static float RadFromDeg(float deg)
{
    return deg * Pi / 180;
}

static float DegFromRad(float rad)
{
    return rad * 180 / Pi;
}

static Vector3 RadFromDeg(const Vector3& vec)
{
    const auto x = RadFromDeg(vec.x);
    const auto y = RadFromDeg(vec.y);
    const auto z = RadFromDeg(vec.z);
    return Vector3{x, y, z};
}

static Vector3 DegFromRad(const Vector3& vec)
{
    const auto x = DegFromRad(vec.x);
    const auto y = DegFromRad(vec.y);
    const auto z = DegFromRad(vec.z);
    return Vector3{x, y, z};
}