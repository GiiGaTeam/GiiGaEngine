module;

#include <memory>
#include <span>
#include <directx/d3dx12.h>
#include <directxtk/SimpleMath.h>

export module ObjectShaderResource;
import RenderDevice;
import RenderContext;
import GPULocalResource;
import Material;

namespace GiiGa
{
    //class Material;

    export class ObjectShaderResource
    {
        public:
        ObjectShaderResource(RenderDevice Device, RenderContext& Context, std::shared_ptr<Material>& Material, bool IsStatic) :
            Material_(Material), IsStatic_(IsStatic)
        {
            UINT SizeInBytes = sizeof(Material::MaterialData);
            D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(0, SizeInBytes);

            Material::MaterialData data = Material_->GetMaterialData();
            const auto MaterialSpan = std::span{reinterpret_cast<uint8_t*>(&data), SizeInBytes};
            
            ConstantBuffer_ = std::make_unique<GPULocalResource>(Device, CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, D3D12_RESOURCE_FLAG_NONE));
            ConstantBuffer_->UpdateContents(Context, MaterialSpan, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            ConstantBufferView_ = ConstantBuffer_->CreateConstantBufferView(desc);
        }

        void UpdateGPUData(RenderContext& Context)
        {
            UINT SizeInBytes = sizeof(Material::MaterialData);
                
            Material::MaterialData data= Material_->GetMaterialData();
            const auto MaterialSpan = std::span{reinterpret_cast<uint8_t*>(&data), SizeInBytes};
            
            if (IsStatic_)
            {
                ConstantBuffer_->UpdateContents(Context, MaterialSpan, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            }
            else
            {
                D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(0, SizeInBytes);
            
                ConstantBufferView_ = Context.AllocateDynamicConstantView(MaterialSpan, 0, desc);
            }
        }

        std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> GetTexturesDescriptors() const
        {
            auto Textures = Material_->GetTextures();
            std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> desc_vector;
            
            for (auto texture : Textures)
            {
                desc_vector.push_back(texture->TextureResource->getDescriptor().getGPUHandle());
            }
            
            return desc_vector;
        }
    private:
        
        bool IsStatic_;
        std::unique_ptr<GPULocalResource> ConstantBuffer_;
        std::shared_ptr<Material> Material_;
        std::shared_ptr<BufferView<Constant>> ConstantBufferView_;
    };
}