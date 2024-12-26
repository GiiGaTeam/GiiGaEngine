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
        typename ViewType::ViewDesc create_desc_;

    public:
        BufferView() = default;

        BufferView(const BufferView& other) = delete;
        BufferView(BufferView&& other) noexcept = default;
        BufferView& operator=(const BufferView& other) = delete;
        BufferView& operator=(BufferView&& other) noexcept = default;

        BufferView(typename ViewType::ViewHolder&& descriptor_holder, typename ViewType::ViewDesc create_desc): holder_(std::move(descriptor_holder)), create_desc_(create_desc)
        {
        }

        const typename ViewType::ViewHolder& getDescriptor()
        {
            return holder_;
        }

        typename ViewType::ViewDesc getCreationDescriptor()
        {
            return create_desc_;
        }
    };
}
