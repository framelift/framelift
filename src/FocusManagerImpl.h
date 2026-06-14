#pragma once
#include <algorithm>
#include <framelift/FocusManager.h>
#include <vector>

class IPlugin;

// Concrete host-side FocusManager. App owns one instance and registers it as a
// service; plugins use it through the abstract FocusManager interface.
class FocusManagerImpl final : public FocusManager
{
public:
    void Acquire(IPlugin* f) noexcept override
    {
        std::erase(stack_, f);
        stack_.push_back(f);
    }

    void Release(IPlugin* f) noexcept override
    {
        std::erase(stack_, f);
    }

    [[nodiscard]] IPlugin* Focused() const noexcept override
    {
        return stack_.empty() ? nullptr : stack_.back();
    }

private:
    std::vector<IPlugin*> stack_;
};
