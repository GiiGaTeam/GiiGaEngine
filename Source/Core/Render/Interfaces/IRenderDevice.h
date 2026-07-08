#pragma once
#include<memory>
#include<list>
#include<vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<nvrhi/nvrhi.h>

#include<unique_any.h>

namespace GiiGa
{
    struct IRenderDevice
    {
        IRenderDevice(int count):
            delete_queue(count)
        {
        }

        virtual ~IRenderDevice()
        {
            for (auto&& vec : delete_queue)
            {
                vec.clear();
            }
        }

        virtual nvrhi::IDevice* NvDevice() = 0;

        void KeepAliveForFramesInFlight(unique_any to_keep_alive)
        {
            delete_queue.back().push_back(std::move(to_keep_alive));
        }

        void DeleteStaleObjects()
        {
            std::vector<unique_any> front = std::move(delete_queue.front());
            delete_queue.pop_front();

            for (auto&& el : front)
            {
                el.reset();
            }
            front.clear();

            delete_queue.push_back(std::move(front));
        }

    protected:
        std::list<std::vector<unique_any>> delete_queue;
    };
}
