export module JsonHints;

import <json/json.h>;

namespace GiiGa
{
    namespace JsonHints
    {
        export template <typename T>
        T FromJson(const Json::Value js);

        export template <typename T>
        Json::Value ToJson(const T& value);

        export template <typename T>
        bool Is(const Json::Value& js);
    }
}
