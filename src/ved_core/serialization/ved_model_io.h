#pragma once

#include "ved_binary_format.h"
#include "ved_document_view_state.h"
#include "ved_font_resolution.h"

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

class TDVecModel;

struct VEDModelWriteResult {
    std::vector<std::byte> bytes;
    VEDBinaryError error = VEDBinaryError::None;

    [[nodiscard]] bool Ok() const noexcept;
};

struct VEDModelReadResult {
    std::unique_ptr<TDVecModel> model;
    VEDBinaryError error = VEDBinaryError::None;
    VEDFontResolutionResult fontResolution;
    VEDDocumentViewState viewState;

    [[nodiscard]] bool Ok() const noexcept;
};

VEDModelWriteResult SaveVecModelToBytes(const TDVecModel& model);
VEDModelWriteResult SaveVecModelToBytes(const TDVecModel& model, const VEDDocumentViewState& viewState);
VEDModelReadResult LoadVecModelFromBytes(std::span<const std::byte> bytes);
VEDModelReadResult LoadVecModelFromBytes(std::span<const std::byte> bytes, TDFontManager& fontManager, TDDocumentID docId = nullptr);
VEDModelReadResult LoadVecModelFromBytes(const void* data, std::size_t size);
VEDModelReadResult LoadVecModelFromBytes(const void* data, std::size_t size, TDFontManager& fontManager, TDDocumentID docId = nullptr);
