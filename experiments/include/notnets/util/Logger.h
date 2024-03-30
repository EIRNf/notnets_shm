#pragma once

// #include <chrono>
#include <boost/chrono/time_point.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <atomic>

namespace notnets
{
    namespace util
    {
        class Logger
        {

        public:
            struct Event
            {
                unsigned long tid; // Thread ID
                boost::chrono::time_point<boost::chrono::high_resolution_clock> timestamp;
                // std::chrono::time_point<std::chrono::high_resolution_clock> timestamp;
                const char *msg;     // Message string
                unsigned long param; // A parameter which can mean anything you want
            };

            Logger();

            inline void Log(const char *msg, unsigned long param, unsigned long id)
            {
                // Get next event index
                unsigned long index = g_pos.fetch_add(1);
                // Write an event at this index
                Event *e = g_events + (index & (BUFFER_SIZE - 1)); // Wrap to buffer size, potentially unsafe
                e->tid = id;                                       // Get thread ID
                e->timestamp = boost::chrono::high_resolution_clock::now();
                e->msg = msg;
                e->param = param;
            }
            Event ReadLogEvent(unsigned long array_pos);

            std::atomic_ulong g_pos;

        private:
            static const unsigned long BUFFER_SIZE = 65536; // Must be a power of 2
            Event g_events[BUFFER_SIZE];
        };
    }
}
// #define LOG(m, p, id) notnets::util::Logger::Log(m, p, id)
