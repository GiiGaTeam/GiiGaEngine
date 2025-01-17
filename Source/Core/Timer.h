#pragma once
#include<chrono>

using duration = std::chrono::duration<double>;
using sys_clock = std::chrono::system_clock;
using time_point = std::chrono::time_point<sys_clock, duration>;

namespace GiiGa
{
    class Timer
    {
    public:
        static void Start()
        {
            start_time_ = std::chrono::time_point_cast<duration>(sys_clock::now());
            prev_frame_time_point_ = start_time_;
        }

        static void UpdateTime()
        {
            auto cur_time = std::chrono::time_point_cast<duration>(sys_clock::now());
            delta_time_ = scale_ * (cur_time - prev_frame_time_point_);
            prev_frame_time_point_ = cur_time;
        }

        static double GetRunTime()
        {
            return (std::chrono::time_point_cast<duration>(sys_clock::now()) - start_time_).count();
        }

        static double GetDeltaTime()
        {
            return delta_time_.count();
        }

    private:
        static inline double scale_ = 1.0;
        static inline time_point start_time_;
        static inline duration delta_time_;
        static inline time_point prev_frame_time_point_ = std::chrono::time_point_cast<duration>(sys_clock::now());
    };
}