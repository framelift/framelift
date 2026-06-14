#pragma once

extern "C"
{
#include <libavcodec/packet.h>
}

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>

// Thread-safe, bounded hand-off of demuxed AVPackets from the demux thread to a
// single decode worker (issue #8, Phase 3). One instance per stream (audio /
// video). The bound caps memory when one stream is read far ahead of the other:
// Push() blocks once the queue is full so the demuxer naturally paces itself.
//
// Header-only and ffmpeg-side (it touches AVPacket), so it is only ever compiled
// into the FFmpeg backend translation units, never the tests.
class FFmpegPacketQueue
{
public:
    explicit FFmpegPacketQueue(std::size_t maxPackets = 256) : maxPackets_(maxPackets)
    {
    }

    ~FFmpegPacketQueue()
    {
        Flush();
    }

    FFmpegPacketQueue(const FFmpegPacketQueue&) = delete;
    FFmpegPacketQueue& operator=(const FFmpegPacketQueue&) = delete;

    // Move pkt's contents into a queue-owned packet (pkt is left unreferenced and
    // reusable by the caller). Blocks while the queue is full. Returns false if
    // the queue was aborted while waiting.
    bool Push(AVPacket* pkt)
    {
        AVPacket* owned = av_packet_alloc();
        if (!owned)
        {
            return false;
        }
        av_packet_move_ref(owned, pkt);

        std::unique_lock lock(m_);
        notFull_.wait(lock, [this] { return abort_ || q_.size() < maxPackets_; });
        if (abort_)
        {
            lock.unlock();
            av_packet_free(&owned);
            return false;
        }
        q_.push(owned);
        lock.unlock();
        notEmpty_.notify_one();
        return true;
    }

    // Move the next packet into out (caller owns it and must av_packet_unref it).
    // Blocks until a packet is available, EOF is signalled, or the queue is
    // aborted. Returns false when the queue is drained at EOF or aborted — the
    // caller distinguishes via Aborted().
    bool Pop(AVPacket* out)
    {
        std::unique_lock lock(m_);
        notEmpty_.wait(lock, [this] { return abort_ || eof_ || !q_.empty(); });
        if (!q_.empty())
        {
            AVPacket* owned = q_.front();
            q_.pop();
            lock.unlock();
            notFull_.notify_one();
            av_packet_move_ref(out, owned);
            av_packet_free(&owned);
            return true;
        }
        // Empty: either EOF (drained) or aborted.
        return false;
    }

    // No more packets will be pushed; Pop() drains the backlog then returns false.
    void SignalEof()
    {
        {
            std::lock_guard lock(m_);
            eof_ = true;
        }
        notEmpty_.notify_all();
    }

    // Wake every waiter; subsequent Push()/Pop() return false immediately. Used to
    // tear down a worker when abandoning the current file.
    void Abort()
    {
        {
            std::lock_guard lock(m_);
            abort_ = true;
        }
        notEmpty_.notify_all();
        notFull_.notify_all();
    }

    // Drop all queued packets and reset eof/abort so the queue can be reused for
    // the next file.
    void Flush()
    {
        std::lock_guard lock(m_);
        while (!q_.empty())
        {
            AVPacket* p = q_.front();
            q_.pop();
            av_packet_free(&p);
        }
        eof_ = false;
        abort_ = false;
    }

    [[nodiscard]] bool Aborted() const
    {
        std::lock_guard lock(m_);
        return abort_;
    }

private:
    mutable std::mutex m_;
    std::condition_variable notEmpty_;
    std::condition_variable notFull_;
    std::queue<AVPacket*> q_;
    std::size_t maxPackets_;
    bool eof_ = false;
    bool abort_ = false;
};
