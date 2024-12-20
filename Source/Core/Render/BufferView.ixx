export module BufferView;

import <memory>;
import <d3d12.h>;
import <iostream>;

export import ViewTypes;
import DescriptorHeap;
import IRenderDevice;

namespace GiiGa
{
    export template <typename ViewType>
    class BufferView
    {
        typename ViewType::ViewHolder holder_;

    public:
        BufferView() = default;

        BufferView(const BufferView& other) = delete;
        BufferView(BufferView&& other) noexcept = default;
        BufferView& operator=(const BufferView& other) = delete;
        BufferView& operator=(BufferView&& other) noexcept = default;

        BufferView(typename ViewType::ViewHolder&& descriptor_holder): holder_(std::move(descriptor_holder))
        {

        }

        const typename ViewType::ViewHolder& getDescriptor()
        {
            return holder_;
        }
    };
}