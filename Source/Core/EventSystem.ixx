module;

#include <functional>
#include <unordered_map>

export module EventSystem;

namespace GiiGa
{
    export template <typename T>
    class EventHandle
    {
    private:
        int id_;
        explicit EventHandle(int id) : id_(id) {}

    public:
        template <typename U>
        friend class EventDispatcher;
    };

    export template <typename T>
    class EventDispatcher
    {
    private:
        inline static int next_id_ = 0;

        std::unordered_map<int, std::function<void(const T&)>> handlers_;

    public:
        EventDispatcher() = default;
        EventDispatcher(const EventDispatcher& other) = delete;
        EventDispatcher& operator=(const EventDispatcher& other) = delete;

        EventHandle<T> Register(std::function<void(const T&)>&& handler)
        {
            auto id = next_id_++;

            handlers_.emplace(id, std::move(handler));

            return EventHandle<T>(id);
        }

        void Unregister(EventHandle<T> id) { handlers_.erase(id.id_); }

        void Invoke(const T& event) const
        {
            for (const auto& [id, handler] : handlers_)
            {
                if (handler)
                {
                    handler(event);
                }
            }
        }
    };
}  // namespace GiiGa