#pragma once

#include "ved_binary_format.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

class TDVecObject;

struct VEDObjectWriteResult {
    std::uint32_t typeFourCC = 0;
    std::vector<std::byte> payload;
    VEDBinaryError error = VEDBinaryError::None;

    [[nodiscard]] bool Ok() const noexcept;
};

struct VEDObjectReadResult {
    std::unique_ptr<TDVecObject> object;
    VEDBinaryError error = VEDBinaryError::None;

    [[nodiscard]] bool Ok() const noexcept;
};

VEDObjectWriteResult SaveVecObjectPayload(const TDVecObject& object);
VEDObjectReadResult LoadVecObjectPayload(std::uint32_t typeFourCC, std::span<const std::byte> payload);

