﻿module;
#include <memory>

export module RenderSystem;

import RenderSystemSettings;
import RenderDevice;
import RenderContext;

namespace GiiGa
{
    export class RenderSystem
    {
    public:
        void Init()
        {
            device_.CreateDevice();
            context_.Create(device_);
        }
    private:
        RenderDevice device_;
        RenderContext context_;
    };
}