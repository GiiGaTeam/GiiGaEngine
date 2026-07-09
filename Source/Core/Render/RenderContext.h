#pragma once

#include <memory>

#include <nvrhi/nvrhi.h>

#include <IRenderContext.h>
#include <RenderDevice.h>

namespace GiiGa
{
    // Frame-recording context. Owns a single graphics command list that is (re)opened
    // at the start of each frame and executed at the end. NVRHI performs automatic
    // resource-state tracking, so there are no explicit barrier/upload helpers here;
    // resource uploads go through nvrhi::ICommandList::writeBuffer / writeTexture.
    class RenderContext : public IRenderContext
    {
    public:
        explicit RenderContext(RenderDevice& device):
            device_(device)
        {
            command_list_ = device_.Device()->createCommandList();
        }

        RenderDevice& GetDevice() { return device_; }

        nvrhi::ICommandList* GetCommandList() override { return command_list_; }

        // Frame lifecycle -------------------------------------------------------
        void OpenFrame()
        {
            command_list_->open();
        }

        void CloseAndExecute()
        {
            command_list_->close();
            nvrhi::ICommandList* list = command_list_;
            device_.Device()->executeCommandLists(&list, 1);
        }

        // Debug markers ---------------------------------------------------------
        void BeginMarker(const char* name) { command_list_->beginMarker(name); }
        void EndMarker() { command_list_->endMarker(); }

    private:
        RenderDevice& device_;
        nvrhi::CommandListHandle command_list_;
    };
}
