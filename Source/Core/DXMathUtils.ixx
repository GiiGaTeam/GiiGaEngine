export module MathUtils;

import <json/config.h>;
import <json/value.h>;
import <iostream>;
import "directxtk12/SimpleMath.h";

export import JsonHints;

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
    
    // note:
    // Do NOT change order of points
    export std::array<Vector3, 8> ExtractFrustumWorldCorners(const Matrix& viewProjMatrix)
    {
        std::array<Vector3, 8> corners_world;
        const auto inv = viewProjMatrix.Invert();
        int i = 0;

        for (int32_t x = 0; x < 2; ++x)
        {
            for (int32_t y = 0; y < 2; ++y)
            {
                for (int32_t z = 0; z < 2; ++z)
                {
                    Vector4 point_ndc = Vector4(
                        2.0f * static_cast<float>(x) - 1.0f,
                        2.0f * static_cast<float>(y) - 1.0f,
                        static_cast<float>(z),
                        1.0f);
                    Vector4 pt = Vector4::Transform(point_ndc, inv);
                    pt = (pt / pt.w);
                    corners_world[i++] = Vector3{pt.x, pt.y, pt.z};
                }
            }
        }

        return corners_world;
    }

    export std::vector<Plane> ExtractFrustumPlanesPointInside(const Matrix& viewProjMatrix)
    {
        std::vector<Plane> planes_world(6);

        auto corners_world = ExtractFrustumWorldCorners(viewProjMatrix);

        // XMPlaneFromPoints negates D result
        // near
        planes_world[0] = DirectX::SimpleMath::Plane(corners_world[0], corners_world[6], corners_world[4]);
        planes_world[0].w *= -1;
        // far
        planes_world[1] = DirectX::SimpleMath::Plane(corners_world[1], corners_world[7], corners_world[3]);
        planes_world[1].w *= -1;
        // left
        planes_world[2] = DirectX::SimpleMath::Plane(corners_world[0], corners_world[1], corners_world[2]);
        planes_world[2].w *= -1;
        //right
        planes_world[3] = DirectX::SimpleMath::Plane(corners_world[5], corners_world[4], corners_world[6]);
        planes_world[3].w *= -1;
        //top
        planes_world[4] = DirectX::SimpleMath::Plane(corners_world[6], corners_world[2], corners_world[3]);
        planes_world[4].w *= -1;
        //bottom
        planes_world[5] = DirectX::SimpleMath::Plane(corners_world[1], corners_world[0], corners_world[4]);
        planes_world[5].w *= -1;

        // Normalize the planes
        for (int i = 0; i < 6; ++i)
        {
            planes_world[i].Normalize();
        }

        return planes_world;
    }

    template<>
    Vector3 JsonHints::FromJson<Vector3>(const Json::Value js)
    {
        return {};
    }
}

