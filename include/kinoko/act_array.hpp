#pragma once
#include "kinoko/act_array.h"
#include <memory>
#include <vector>
namespace kinoko {
using ActArray = std::vector<void*>;
// Commit is allocation-free: callers can prepare all hierarchy changes first.
void replace_act_array(void* slot, std::unique_ptr<ActArray> values) noexcept;
}
