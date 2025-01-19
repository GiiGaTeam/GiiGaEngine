#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <directx/d3d12.h>
#include<directx/d3dx12_core.h>


#include<vector>
#include<memory>
#include<iostream>
#include<functional>

#include<RenderDevice.h>
#include<ShaderManager.h>
#include<PerObjectData.h>
#include<IObjectShaderResource.h>
#include<RenderContext.h>

namespace GiiGa
{
    class PSO
    {
    public:
        PSO() = default;

        void GeneratePSO(RenderDevice& device, int cbv_num, int srv_num = 0, int uav_num = 0)
        {
            // here
            // Define descriptor ranges
            std::vector<D3D12_DESCRIPTOR_RANGE> ranges(cbv_num + srv_num + uav_num);
            for (int i = 0; i < cbv_num; ++i)
            {
                ranges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                ranges[i].NumDescriptors = 1;
                ranges[i].BaseShaderRegister = i;
                ranges[i].RegisterSpace = 0;
                ranges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
            }
            for (int i = 0; i < srv_num; ++i)
            {
                ranges[cbv_num + i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                ranges[cbv_num + i].NumDescriptors = 1;
                ranges[cbv_num + i].BaseShaderRegister = i;
                ranges[cbv_num + i].RegisterSpace = 0;
                ranges[cbv_num + i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
            }
            for (int i = 0; i < uav_num; ++i)
            {
                ranges[cbv_num + srv_num + i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                ranges[cbv_num + srv_num + i].NumDescriptors = 1;
                ranges[cbv_num + srv_num + i].BaseShaderRegister = i;
                ranges[cbv_num + srv_num + i].RegisterSpace = 0;
                ranges[cbv_num + srv_num + i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
            }

            // Create root parameters
            std::vector<D3D12_ROOT_PARAMETER> rootParameters(ranges.size());
            for (size_t i = 0; i < ranges.size(); ++i)
            {
                rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                rootParameters[i].DescriptorTable.NumDescriptorRanges = 1;
                rootParameters[i].DescriptorTable.pDescriptorRanges = &ranges[i];
                rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            }

            // Create the root signature description
            D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
            rootSigDesc.NumParameters = static_cast<UINT>(rootParameters.size());
            rootSigDesc.pParameters = rootParameters.data();
            rootSigDesc.NumStaticSamplers = static_samplers_.size();
            rootSigDesc.pStaticSamplers = static_samplers_.data();
            rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
            // create a root signature with a single slot which points to a
            // descriptor range consisting of a single constant buffer.
            ID3DBlob* serializedRootSig = nullptr;
            ID3DBlob* errorBlob = nullptr;
            HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc,
                                                     D3D_ROOT_SIGNATURE_VERSION_1,
                                                     &serializedRootSig,
                                                     &errorBlob);

            if (FAILED(hr))
            {
                // If the shader failed to compile it should have written something to the error message.
                if (errorBlob)
                {
                    char* compileErrors = (char*)(errorBlob->GetBufferPointer());
                    std::cout << compileErrors << std::endl;
                }
            }


            mRootSignature = device.CreateRootSignature(0, serializedRootSig);

            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{
                mRootSignature.get(), VS_, PS_, DS_, HS_, GS_, stream_output_, blend_state_, sample_mask_,
                rasterizer_state_, depth_stencil_state_, input_layout_, strip_cut_value_, primitive_topology_type_, NumRenderTargets_, {},
                DSVFormat_, sample_desc_, node_mask_, cashed_pso_, flags_
            };

            for (UINT i = 0; i < psoDesc.NumRenderTargets; ++i)
            {
                psoDesc.RTVFormats[i] = RTVFormat_[i];
            }

            state_ = device.CreateGraphicsPipelineState(psoDesc);
        }

        //=============================SETTING PSO PARAMETERS=============================

        PSO& set_vs(const D3D12_SHADER_BYTECODE& vs)
        {
            VS_ = vs;
            return *this;
        }

        PSO& set_ps(const D3D12_SHADER_BYTECODE& ps)
        {
            PS_ = ps;
            return *this;
        }

        PSO& set_ds(const D3D12_SHADER_BYTECODE& ds)
        {
            DS_ = ds;
            return *this;
        }

        PSO& set_hs(const D3D12_SHADER_BYTECODE& hs)
        {
            HS_ = hs;
            return *this;
        }

        PSO& set_gs(const D3D12_SHADER_BYTECODE& gs)
        {
            GS_ = gs;
            return *this;
        }

        PSO& set_stream_output(const D3D12_STREAM_OUTPUT_DESC& stream_output)
        {
            stream_output_ = stream_output;
            return *this;
        }

        PSO& set_blend_state(const D3D12_BLEND_DESC& blend_state)
        {
            blend_state_ = blend_state;
            return *this;
        }

        PSO& set_sample_mask(UINT sample_mask)
        {
            sample_mask_ = sample_mask;
            return *this;
        }

        PSO& set_rasterizer_state(const D3D12_RASTERIZER_DESC& rasterizer_state)
        {
            rasterizer_state_ = rasterizer_state;
            return *this;
        }

        PSO& set_depth_stencil_state(const D3D12_DEPTH_STENCIL_DESC& depth_stencil_state)
        {
            depth_stencil_state_ = depth_stencil_state;
            return *this;
        }

        PSO& set_input_layout(const D3D12_INPUT_LAYOUT_DESC& input_layout)
        {
            input_layout_ = input_layout;
            return *this;
        }

        PSO& SetBufferStripCutValue(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE StripCutValue)
        {
            strip_cut_value_ = StripCutValue;
            return *this;
        }

        PSO& set_primitive_topology_type(D3D12_PRIMITIVE_TOPOLOGY_TYPE primitive_topology_type)
        {
            primitive_topology_type_ = primitive_topology_type;
            return *this;
        }

        PSO& set_rtv_format(DXGI_FORMAT rtv_format[], size_t num_render_targets)
        {
            NumRenderTargets_ = num_render_targets;
            for (size_t i = 0; i < num_render_targets; ++i)
            {
                RTVFormat_[i] = rtv_format[i];
            }
            return *this;
        }

        PSO& set_dsv_format(DXGI_FORMAT dsv_format)
        {
            DSVFormat_ = dsv_format;
            return *this;
        }

        PSO& set_sample_desc(const DXGI_SAMPLE_DESC& sample_desc)
        {
            sample_desc_ = sample_desc;
            return *this;
        }

        PSO& SetNodeMask(UINT nodeMask)
        {
            node_mask_ = nodeMask;
            return *this;
        }

        PSO& SetCashedPso(const D3D12_CACHED_PIPELINE_STATE& cashedPso)
        {
            cashed_pso_ = cashedPso;
            return *this;
        }

        PSO& SetFlags(D3D12_PIPELINE_STATE_FLAGS flags)
        {
            flags_ = flags;
            return *this;
        }

        PSO& add_static_samplers(D3D12_STATIC_SAMPLER_DESC desc)
        {
            static_samplers_.push_back(desc);
            return *this;
        }

        //PSO& add_custom_samplers(std::vector<D3D12_STATIC_SAMPLER_DESC> sampler_descs)
        //{
        //    samplers_.insert(samplers_.end(), sampler_descs.begin(), sampler_descs.end());
        //    return *this;
        //}

        PSO& SetPerObjectDataFunction(const std::function<void(RenderContext& context, PerObjectData& per_obj)>& per_object_data_function)
        {
            set_per_obj_data_fn_ = per_object_data_function;
            return *this;
        }

        PSO& SetShaderResourceFunction(std::function<void(RenderContext& context, IObjectShaderResource& resource)> shared_res_function)
        {
            set_obj_shared_res_fn_ = shared_res_function;
            return *this;
        }

        void SetPerObjectData(RenderContext& context, PerObjectData& data)
        {
            set_per_obj_data_fn_(context, data);
        }

        void SetShaderResources(RenderContext& context, IObjectShaderResource& data)
        {
            set_obj_shared_res_fn_(context, data);
        }

        std::shared_ptr<ID3D12PipelineState> GetState() { return state_; }

        std::shared_ptr<ID3D12RootSignature> GetSignature()
        {
            return mRootSignature;
        }

    private:
        D3D12_SHADER_BYTECODE VS_ = {};
        D3D12_SHADER_BYTECODE PS_ = {};
        D3D12_SHADER_BYTECODE DS_ = {};
        D3D12_SHADER_BYTECODE HS_ = {};
        D3D12_SHADER_BYTECODE GS_ = {};
        D3D12_STREAM_OUTPUT_DESC stream_output_ = {};
        D3D12_BLEND_DESC blend_state_ = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        UINT sample_mask_ = UINT_MAX;
        D3D12_RASTERIZER_DESC rasterizer_state_ = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        D3D12_DEPTH_STENCIL_DESC depth_stencil_state_ = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        D3D12_INPUT_LAYOUT_DESC input_layout_ = {};
        D3D12_INDEX_BUFFER_STRIP_CUT_VALUE strip_cut_value_ = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
        D3D12_PRIMITIVE_TOPOLOGY_TYPE primitive_topology_type_ = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        UINT NumRenderTargets_ = 1;
        DXGI_FORMAT RTVFormat_[8] = {
            DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM
        };
        DXGI_FORMAT DSVFormat_ = DXGI_FORMAT_D24_UNORM_S8_UINT;
        DXGI_SAMPLE_DESC sample_desc_ = DXGI_SAMPLE_DESC{1, 0};
        UINT node_mask_ = 0;
        D3D12_CACHED_PIPELINE_STATE cashed_pso_ = {};
        D3D12_PIPELINE_STATE_FLAGS flags_ = D3D12_PIPELINE_STATE_FLAG_NONE;

        std::function<void(RenderContext& context, PerObjectData& per_obj)> set_per_obj_data_fn_;

        std::function<void(RenderContext& context, IObjectShaderResource& resource)> set_obj_shared_res_fn_;

        std::vector<D3D12_STATIC_SAMPLER_DESC> static_samplers_;

        std::shared_ptr<ID3D12RootSignature> mRootSignature;

        std::shared_ptr<ID3D12PipelineState> state_;
    };
}
