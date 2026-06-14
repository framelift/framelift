#pragma once

// Thin query interface for the History plugin — the two lookups other
// components need an immediate answer for. History *updates* flow through
// events (FileOpenedEvent / FileEndedEvent), not through this interface.
class IHistory
{
public:
    static constexpr const char* InterfaceId = "framelift.IHistory";
    virtual ~IHistory() = default;

    // Returns the most recently played path. Returns empty string ("") if history
    // is empty. The pointer is valid for the duration of the call.
    // Use buf/cap to copy: returns total length excl. NUL; buf may be null.
    [[nodiscard]] virtual int GetMostRecent(char* buf, int cap) const noexcept = 0;

    // Return the saved resume position for `path`, or 0.0 if not found.
    [[nodiscard]] virtual double GetResumePos(const char* path) const noexcept = 0;
};