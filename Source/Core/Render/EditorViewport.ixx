module;

export module EditorViewport;

export import Viewport;

namespace GiiGa
{
    export class EditorViewport:public Viewport
    {
    public:
        ::GiiGa::DescriptorHeapAllocation getCameraDescriptor() override;
    };
}
