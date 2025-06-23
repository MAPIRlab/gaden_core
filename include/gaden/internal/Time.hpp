#pragma once
#include <chrono>
#include <cmath>
#include <thread>

namespace gaden::Utils::Time
{
    typedef std::chrono::high_resolution_clock Clock;
    typedef std::chrono::_V2::system_clock::duration Duration;
    typedef std::chrono::_V2::system_clock::time_point TimePoint;

    inline double toSeconds(Duration duration)
    {
        constexpr double nanoToSec = 1e-9;
        return duration.count() * nanoToSec;
    }

    struct Stopwatch
    {
        Clock clock;
        TimePoint start;

        Stopwatch()
            : clock(), start(clock.now())
        {
        }

        double ellapsed()
        {
            return toSeconds(clock.now() - start);
        }

        void restart()
        {
            start = clock.now();
        }
    };

    struct Countdown
    {
    public:
        Countdown()
            : clock(), start(clock.now()), length(0)
        {}

        Countdown(double _length)
            : clock(), start(clock.now()), length(_length)
        {}

        void Restart()
        {
            start = clock.now();
        }

        void Restart(double _length)
        {
            start = clock.now();
            length = _length;
        }

        bool isDone()
        {
            return toSeconds(clock.now() - start) >= length;
        }

        float timeRemaining()
        {
            return toSeconds(clock.now() - start);
        }

        float proportionComplete()
        {
            if (length <= 0)
                return 1;
            else
                return timeRemaining() / length;
        }

    private:
        Clock clock;
        TimePoint start;
        double length;
    };

    // copied from rclcpp::Rate!
    // I want to be able to use it without ROS
    struct Rate
    {
    public:
        explicit Rate(double rate)
            : period_(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(1.0 / rate)))
        {}

        explicit Rate(std::chrono::nanoseconds period)
            : period_(period), last_interval_(Clock::now())
        {}

        virtual bool
        sleep()
        {
            // Time coming into sleep
            auto now = Clock::now();
            // Time of next interval
            auto next_interval = last_interval_ + period_;
            // Detect backwards time flow
            if (now < last_interval_)
            {
                // Best thing to do is to set the next_interval to now + period
                next_interval = now + period_;
            }
            // Calculate the time to sleep
            auto time_to_sleep = next_interval - now;
            // Update the interval
            last_interval_ += period_;
            // If the time_to_sleep is negative or zero, don't sleep
            if (time_to_sleep <= std::chrono::seconds(0))
            {
                // If an entire cycle was missed then reset next interval.
                // This might happen if the loop took more than a cycle.
                // Or if time jumps forward.
                if (now > next_interval + period_)
                {
                    last_interval_ = now + period_;
                }
                // Either way do not sleep and return false
                return false;
            }
            // Sleep (will get interrupted by ctrl-c, may not sleep full time)
            std::this_thread::sleep_for(time_to_sleep);
            return true;
        }

    private:
        std::chrono::nanoseconds period_;
        using ClockDurationNano = std::chrono::duration<typename Clock::rep, std::nano>;
        std::chrono::time_point<Clock, ClockDurationNano> last_interval_;
    };
} // namespace gaden::Utils::Time