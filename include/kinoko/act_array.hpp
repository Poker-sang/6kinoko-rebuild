#pragma once
#include "kinoko/act_array.h"
#include <memory>
#include <vector>
namespace kinoko {
using ActArray = std::vector<int32_t>;
// Commit is allocation-free: callers can prepare all hierarchy changes first.
void replace_act_array(int32_t slot, std::unique_ptr<ActArray> values) noexcept;
}
