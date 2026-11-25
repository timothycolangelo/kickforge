#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace kf
{
struct KickBuffer
{
    std::vector<float> samples;
    double sampleRate = 0.0;
    std::uint64_t hash = 0;
    int crossfadeSample = 0;
};

// Hands finished renders from the render thread to the audio thread.
//
// The audio thread never locks, allocates or frees: it reads the current
// pointer and publishes it as a hazard pointer before using it. Buffers are
// only freed on the publishing (non-audio) thread, and never while they are
// the audio thread's hazard.
class KickBufferStore
{
public:
    ~KickBufferStore() { current.store (nullptr); }

    // Non-audio threads only.
    void publish (std::unique_ptr<KickBuffer> buffer)
    {
        const std::scoped_lock lock (poolMutex);
        auto* raw = buffer.get();
        pool.push_back (std::move (buffer));
        current.store (raw);
        collectGarbage();
    }

    // Audio thread. Returns the newest buffer (or nullptr) and keeps it alive
    // until the next call.
    const KickBuffer* acquireForPlayback() noexcept
    {
        auto* p = current.load();
        for (;;)
        {
            hazard.store (p);
            auto* q = current.load();
            if (q == p)
                return p;
            p = q;
        }
    }

    // Any thread: the newest buffer, for display. Valid only while the
    // caller holds no expectation of it outliving the next publish; the GUI
    // copies what it needs under lockForReading().
    std::unique_lock<std::mutex> lockForReading() { return std::unique_lock (poolMutex); }
    const KickBuffer* latestUnsafe() const noexcept { return current.load(); }

private:
    void collectGarbage()
    {
        auto* cur = current.load();
        auto* haz = hazard.load();
        std::erase_if (pool, [cur, haz] (const std::unique_ptr<KickBuffer>& b)
                       { return b.get() != cur && b.get() != haz; });
    }

    std::atomic<KickBuffer*> current { nullptr };
    std::atomic<const KickBuffer*> hazard { nullptr };
    std::mutex poolMutex;
    std::vector<std::unique_ptr<KickBuffer>> pool;
};
} // namespace kf
