#pragma once

#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>

class EventQueue {
public:
    void post(std::function<void()> event) {
        std::lock_guard lock(m_mutex);
        m_events.push(std::move(event));
    }

    std::size_t process(std::size_t maxEvents = 256) {
        std::size_t processed = 0;
        while (processed < maxEvents) {
            std::function<void()> event;
            { std::lock_guard lock(m_mutex); if (m_events.empty()) break; event = std::move(m_events.front()); m_events.pop(); }
            event();
            ++processed;
        }
        return processed;
    }

private:
    std::queue<std::function<void()>> m_events;
    std::mutex m_mutex;
};
