export module MathUtils;

import <json/config.h>;
import <json/value.h>;

import "directxtk12/SimpleMath.h";

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
        return vec * Pi / 180;
    }

    export Vector3 DegFromRad(const Vector3& vec)
    {
        return vec * 180 / Pi;
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
        std::vector<Plane> planes(6);
        std::array<Vector3, 8> corners;
        const auto inv = viewProjMatrix.Invert();
        int i = 0;

        for (int32_t x = 0; x < 2; ++x)
        {
            for (int32_t y = 0; y < 2; ++y)
            {
                for (int32_t z = 0; z < 2; ++z)
                {
                    Vector4 pt = Vector4::Transform(Vector4(
                                                        2.0f * static_cast<float>(x) - 1.0f,
                                                        2.0f * static_cast<float>(y) - 1.0f,
                                                        static_cast<float>(z),
                                                        1.0f), inv);
                    pt = (pt / pt.w);
                    corners[i++] = Vector3{pt.x, pt.y, pt.z};
                }
            }
        }

        // near
        planes[0] = DirectX::SimpleMath::Plane(corners[0], corners[6], corners[4]);
        // far
        planes[1] = DirectX::SimpleMath::Plane(corners[1], corners[7], corners[3]);
        // left
        planes[2] = DirectX::SimpleMath::Plane(corners[0], corners[1], corners[2]);
        //right
        planes[3] = DirectX::SimpleMath::Plane(corners[5], corners[4], corners[6]);
        //top
        planes[4] = DirectX::SimpleMath::Plane(corners[6], corners[2], corners[3]);
        //bottom
        planes[5] = DirectX::SimpleMath::Plane(corners[1], corners[0], corners[4]);

        // Normalize the planes
        for (int i = 0; i < 6; ++i)
        {
            planes[i].Normalize();
        }

        return planes;
    }
}
