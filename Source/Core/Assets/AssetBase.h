#pragma once

#include<AssetHandle.h>
#include<EventSystem.h>

namespace GiiGa
{
    class AssetBase
    {
    private:
        AssetHandle id_;

    public:
        AssetBase() = default;

        AssetBase(const AssetBase&) = delete;
        AssetBase& operator=(const AssetBase&) = delete;

        AssetBase(AssetBase&& other) noexcept
            : id_(std::move(other.id_)),
              OnDestroy(std::move(other.OnDestroy)),
              OnUpdate(std::move(other.OnUpdate))
        
        {
            other.id_ = AssetHandle{};
        }

        AssetBase& operator=(AssetBase&& other) noexcept
        {
            if (this != &other)
            {
                id_ = std::move(other.id_);
                OnDestroy = std::move(other.OnDestroy);
                OnUpdate = std::move(other.OnUpdate);
                other.id_ = AssetHandle{};
            }
            return *this;
        }

        AssetBase(AssetHandle uuid)
            : id_(uuid)
        {
        }

        AssetHandle GetId() const
        {
            return id_;
        }

        virtual AssetType GetType() = 0;

        EventDispatcher<AssetHandle> OnDestroy;

        EventDispatcher<AssetHandle> OnUpdate;

        virtual ~AssetBase()
        {
            OnDestroy.Invoke(id_);
        }
    };
} // namespace GiiGa
