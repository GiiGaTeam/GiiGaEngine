module;

export module Viewport;

import RenderPass;
import GPULocalResource;
import DescriptorHeap;

namespace GiiGa
{
    export class Viewport
    {
    public:
        //Viewport()
        //{
        //    
        //}
        
        virtual ~Viewport() = default;

        virtual DescriptorHeapAllocation getCameraDescriptor() =0;

        void Execute()
        {
            
        }
    private:
        GPULocalResource resultResource_;
    };
}
