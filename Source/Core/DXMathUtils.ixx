module;

#include <json/config.h>
#include <json/value.h>

#include "directxtk12/SimpleMath.h"

export module MathUtils;

using namespace DirectX::SimpleMath;

namespace GiiGa
{
    export constexpr float Pi = 3.14159265f;

    export float RadFromDeg(float deg)
    {
        return deg * Pi / 180;
    }

    export float DegFromRad(float rad)
    {
        return rad * 180 / Pi;
    }

    export Vector3 RadFromDeg(const Vector3& vec)
    {
        const auto x = RadFromDeg(vec.x);
        const auto y = RadFromDeg(vec.y);
        const auto z = RadFromDeg(vec.z);
        return Vector3{x, y, z};
    }

    export Vector3 DegFromRad(const Vector3& vec)
    {
        const auto x = DegFromRad(vec.x);
        const auto y = DegFromRad(vec.y);
        const auto z = DegFromRad(vec.z);
        return Vector3{x, y, z};
    }

    export Json::Value Vector3ToJson(const Vector3& vec)
    {
        Json::Value json;

        json["x"] = vec.x;
        json["y"] = vec.y;
        json["z"] = vec.z;

        return json;
    }

    export Vector3 Vector3FromJson(const Json::Value& json)
    {
        Vector3 vec;

        vec.x = json["x"].asFloat();
        vec.y = json["y"].asFloat();
        vec.z = json["z"].asFloat();

        return vec;
    }

    // https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
    export std::vector<Plane> ExtractFrustumPlanesPointInside(const Matrix& viewProjMatrix)
    {
        std::vector<Plane> planes;

        // Left plane
        planes[0] = Plane(viewProjMatrix._14 + viewProjMatrix._11,
                          viewProjMatrix._24 + viewProjMatrix._21,
                          viewProjMatrix._34 + viewProjMatrix._31,
                          viewProjMatrix._44 + viewProjMatrix._41);
        // Right plane
        planes[1] = Plane(viewProjMatrix._14 - viewProjMatrix._11,
                          viewProjMatrix._24 - viewProjMatrix._21,
                          viewProjMatrix._34 - viewProjMatrix._31,
                          viewProjMatrix._44 - viewProjMatrix._41);
        // Top plane
        planes[2] = Plane(viewProjMatrix._14 - viewProjMatrix._12,
                          viewProjMatrix._24 - viewProjMatrix._22,
                          viewProjMatrix._34 - viewProjMatrix._32,
                          viewProjMatrix._44 - viewProjMatrix._42);
        // Bottom plane
        planes[3] = Plane(viewProjMatrix._14 + viewProjMatrix._12,
                          viewProjMatrix._24 + viewProjMatrix._22,
                          viewProjMatrix._34 + viewProjMatrix._32,
                          viewProjMatrix._44 + viewProjMatrix._42);
        // Near plane
        planes[4] = Plane(viewProjMatrix._13,
                          viewProjMatrix._23,
                          viewProjMatrix._33,
                          viewProjMatrix._43);
        // Far plane
        planes[5] = Plane(viewProjMatrix._14 - viewProjMatrix._13,
                          viewProjMatrix._24 - viewProjMatrix._23,
                          viewProjMatrix._34 - viewProjMatrix._33,
                          viewProjMatrix._44 - viewProjMatrix._43);

        // Normalize the planes
        for (int i = 0; i < 6; ++i)
        {
            planes[i].Normalize();
        }

        return planes;
    }
}
