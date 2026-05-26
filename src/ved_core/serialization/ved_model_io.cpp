#include "ved_model_io.h"

#include "ved_binary_reader.h"
#include "ved_binary_writer.h"
#include "ved_object_io.h"
#include "vec_document_settings.h"
#include "vec_model.h"
#include "vec_object.h"

#include <cstdint>
#include <limits>

namespace {

constexpr std::uint32_t kVEDModelMagic = VEDMakeFourCC('V', 'E', 'D', 'M');
constexpr std::uint16_t kVEDModelVersionMajor = 1;
constexpr std::uint16_t kVEDModelVersionMinor = 0;
constexpr std::uint16_t kVEDModelVersionPatch = 2;
constexpr std::uint32_t kVEDModelMetaChunk = VEDMakeFourCC('M', 'E', 'T', 'A');
constexpr std::uint32_t kVEDModelSettingsChunk = VEDMakeFourCC('D', 'S', 'E', 'T');
constexpr std::uint32_t kVEDModelViewChunk = VEDMakeFourCC('V', 'I', 'E', 'W');
constexpr std::uint32_t kVEDModelObjectsChunk = VEDMakeFourCC('O', 'B', 'J', 'S');

void writePoint(VEDBinaryWriter& writer, TDMatPoint point)
{
    writer.WriteDouble(point.x);
    writer.WriteDouble(point.y);
}

TDMatPoint readPoint(VEDBinaryReader& reader)
{
    TDMatPoint point;
    point.x = reader.ReadDouble();
    point.y = reader.ReadDouble();
    return point;
}

void writeViewState(VEDBinaryWriter& writer, const VEDDocumentViewState& viewState)
{
    writer.WriteDouble(viewState.zoom);
    writePoint(writer, viewState.center);
    writer.WriteBool(viewState.showGrid);
    writer.WriteBool(viewState.gridLock);
    writer.WriteBool(viewState.showRulers);
}

VEDDocumentViewState readViewState(VEDBinaryReader& reader)
{
    VEDDocumentViewState viewState;
    viewState.present = true;
    viewState.zoom = reader.ReadDouble();
    viewState.center = readPoint(reader);
    viewState.showGrid = reader.ReadBool();
    viewState.gridLock = reader.ReadBool();
    viewState.showRulers = reader.ReadBool();
    return viewState;
}

void writeDocumentSettings(VEDBinaryWriter& writer, const TDVecDocumentSettings& settings)
{
    writer.WriteDouble(settings.unitSettings.realUnitsPerMillimeter);
    writer.WriteUInt8(static_cast<std::uint8_t>(settings.unitSettings.displayUnit));
    writer.WriteDouble(settings.gridSettings.majorStepReal);
    writer.WriteInt32(settings.gridSettings.subdivisions);
    writer.WriteInt32(static_cast<std::int32_t>(settings.gridSettings.resolutionLimitPixels));
    writer.WriteCString(settings.pageSettings.formatName);
    writer.WriteDouble(settings.pageSettings.widthReal);
    writer.WriteDouble(settings.pageSettings.heightReal);
    writer.WriteDouble(settings.pageSettings.pageOriginX);
    writer.WriteDouble(settings.pageSettings.pageOriginY);
    writer.WriteUInt8(static_cast<std::uint8_t>(settings.pageSettings.orientation));
}

TDVecDocumentSettings readDocumentSettings(VEDBinaryReader& reader)
{
    TDVecDocumentSettings settings;
    settings.unitSettings.realUnitsPerMillimeter = reader.ReadDouble();
    const std::uint8_t displayUnit = reader.ReadUInt8();
    if (displayUnit <= static_cast<std::uint8_t>(TDVecDisplayUnit::Inch)) {
        settings.unitSettings.displayUnit = static_cast<TDVecDisplayUnit>(displayUnit);
    }
    settings.gridSettings.majorStepReal = reader.ReadDouble();
    settings.gridSettings.subdivisions = reader.ReadInt32();
    settings.gridSettings.resolutionLimitPixels = static_cast<long>(reader.ReadInt32());
    settings.pageSettings.formatName = reader.ReadCString();
    settings.pageSettings.widthReal = reader.ReadDouble();
    settings.pageSettings.heightReal = reader.ReadDouble();
    settings.pageSettings.pageOriginX = reader.ReadDouble();
    settings.pageSettings.pageOriginY = reader.ReadDouble();
    const std::uint8_t orientation = reader.ReadUInt8();
    if (orientation <= static_cast<std::uint8_t>(TDVecPageOrientation::Landscape)) {
        settings.pageSettings.orientation = static_cast<TDVecPageOrientation>(orientation);
    }
    return settings;
}

bool writeChunk(VEDBinaryWriter& writer, std::uint32_t chunkFourCC, std::span<const std::byte> payload)
{
    if(payload.size() > std::numeric_limits<std::uint32_t>::max()) {
        return false;
    }

    writer.WriteFourCC(chunkFourCC);
    writer.WriteUInt32(static_cast<std::uint32_t>(payload.size()));
    writer.WriteBytes(payload);
    return true;
}

void writeHeader(VEDBinaryWriter& writer)
{
    writer.WriteFourCC(kVEDModelMagic);
    writer.WriteUInt16(kVEDModelVersionMajor);
    writer.WriteUInt16(kVEDModelVersionMinor);
    writer.WriteUInt16(kVEDModelVersionPatch);
}

bool readHeader(VEDBinaryReader& reader)
{
    if(reader.ReadFourCC() != kVEDModelMagic) {
        reader.Fail(VEDBinaryError::InvalidSignature);
        return false;
    }

    const std::uint16_t major = reader.ReadUInt16();
    const std::uint16_t minor = reader.ReadUInt16();
    const std::uint16_t patch = reader.ReadUInt16();
    if(!reader.Ok()) {
        return false;
    }

    if(major != kVEDModelVersionMajor || minor != kVEDModelVersionMinor || patch != kVEDModelVersionPatch) {
        reader.Fail(VEDBinaryError::UnsupportedVersion);
        return false;
    }

    return true;
}

} // namespace

bool VEDModelWriteResult::Ok() const noexcept
{
    return error == VEDBinaryError::None;
}

bool VEDModelReadResult::Ok() const noexcept
{
    return error == VEDBinaryError::None && model != nullptr;
}

VEDModelWriteResult SaveVecModelToBytes(const TDVecModel& model)
{
    return SaveVecModelToBytes(model, {});
}

VEDModelWriteResult SaveVecModelToBytes(const TDVecModel& model, const VEDDocumentViewState& viewState)
{
    VEDModelWriteResult result;
    VEDBinaryWriter writer;
    writeHeader(writer);

    VEDBinaryWriter metadataWriter;
    writePoint(metadataWriter, model.GetTopLeftArea());
    writePoint(metadataWriter, model.GetBottomRightArea());
    metadataWriter.WriteUInt32(static_cast<std::uint32_t>(model.GetDefaultColor()));
    if(!writeChunk(writer, kVEDModelMetaChunk, metadataWriter.Bytes())) {
        result.error = VEDBinaryError::InvalidArgument;
        return result;
    }

    VEDBinaryWriter settingsWriter;
    writeDocumentSettings(settingsWriter, model.DocumentSettings());
    if(!writeChunk(writer, kVEDModelSettingsChunk, settingsWriter.Bytes())) {
        result.error = VEDBinaryError::InvalidArgument;
        return result;
    }

    if(viewState.present) {
        VEDBinaryWriter viewWriter;
        writeViewState(viewWriter, viewState);
        if(!writeChunk(writer, kVEDModelViewChunk, viewWriter.Bytes())) {
            result.error = VEDBinaryError::InvalidArgument;
            return result;
        }
    }

    VEDBinaryWriter objectsWriter;
    objectsWriter.WriteUInt32(static_cast<std::uint32_t>(model.CountObjects()));
    for(int index = 0; index < model.CountObjects(); ++index) {
        const TDVecObject* object = model.GetObject(index);
        if(!object) {
            result.error = VEDBinaryError::InvalidArgument;
            return result;
        }

        const VEDObjectWriteResult objectResult = SaveVecObjectPayload(*object);
        if(!objectResult.Ok()) {
            result.error = objectResult.error;
            return result;
        }
        if(objectResult.payload.size() > std::numeric_limits<std::uint32_t>::max()) {
            result.error = VEDBinaryError::InvalidArgument;
            return result;
        }

        objectsWriter.WriteFourCC(objectResult.typeFourCC);
        objectsWriter.WriteUInt32(static_cast<std::uint32_t>(objectResult.payload.size()));
        objectsWriter.WriteBytes(objectResult.payload);
    }

    if(!writeChunk(writer, kVEDModelObjectsChunk, objectsWriter.Bytes())) {
        result.error = VEDBinaryError::InvalidArgument;
        return result;
    }

    result.bytes = writer.Buffer();
    return result;
}

VEDModelReadResult LoadVecModelFromBytes(std::span<const std::byte> bytes)
{
    VEDBinaryReader reader(bytes);
    if(!readHeader(reader)) {
        return {nullptr, reader.Error(), {}, {}};
    }

    auto model = std::make_unique<TDVecModel>();
    VEDDocumentViewState viewState;
    bool hasMetadata = false;
    bool hasObjects = false;

    while(!reader.End()) {
        const std::uint32_t chunkFourCC = reader.ReadFourCC();
        const std::uint32_t payloadSize = reader.ReadUInt32();
        const std::span<const std::byte> payload = reader.ReadBytes(payloadSize);
        if(!reader.Ok()) {
            return {nullptr, reader.Error(), {}, {}};
        }

        VEDBinaryReader chunkReader(payload);
        switch(chunkFourCC) {
        case kVEDModelMetaChunk: {
            const TDMatPoint topLeftArea = readPoint(chunkReader);
            const TDMatPoint bottomRightArea = readPoint(chunkReader);
            const TDRgbColor defaultColor = static_cast<TDRgbColor>(chunkReader.ReadUInt32());
            if(!chunkReader.Ok()) {
                return {nullptr, chunkReader.Error(), {}, {}};
            }
            model->SetTopLeftArea(topLeftArea);
            model->SetBottomRightArea(bottomRightArea);
            model->SetDefaultColor(defaultColor);
            hasMetadata = true;
            break;
        }
        case kVEDModelSettingsChunk:
            {
                const TDVecDocumentSettings candidate = readDocumentSettings(chunkReader);
                if (chunkReader.Ok()) {
                    model->SetDocumentSettings(candidate);
                }
            }
            break;
        case kVEDModelViewChunk:
            {
                const VEDDocumentViewState candidate = readViewState(chunkReader);
                viewState = chunkReader.Ok() ? candidate : VEDDocumentViewState{};
            }
            break;
        case kVEDModelObjectsChunk: {
            const std::uint32_t objectCount = chunkReader.ReadUInt32();
            if(!chunkReader.Ok()) {
                return {nullptr, chunkReader.Error(), {}, {}};
            }
            if(objectCount > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
                return {nullptr, VEDBinaryError::InvalidArgument, {}, {}};
            }

            for(std::uint32_t index = 0; index < objectCount; ++index) {
                const std::uint32_t typeFourCC = chunkReader.ReadFourCC();
                const std::uint32_t objectPayloadSize = chunkReader.ReadUInt32();
                const std::span<const std::byte> objectPayload = chunkReader.ReadBytes(objectPayloadSize);
                if(!chunkReader.Ok()) {
                    return {nullptr, chunkReader.Error(), {}, {}};
                }

                VEDObjectReadResult objectResult = LoadVecObjectPayload(typeFourCC, objectPayload);
                if(!objectResult.Ok()) {
                    return {nullptr, objectResult.error, {}, {}};
                }
                model->AppendObject(objectResult.object.release());
            }
            hasObjects = true;
            break;
        }
        default:
            break;
        }
    }

    if(!hasMetadata || !hasObjects) {
        return {nullptr, VEDBinaryError::InvalidArgument, {}, {}};
    }

    model->SetChanged(false);
    return {std::move(model), VEDBinaryError::None, {}, viewState};
}

VEDModelReadResult LoadVecModelFromBytes(std::span<const std::byte> bytes, TDFontManager& fontManager, TDDocumentID docId)
{
    VEDModelReadResult result = LoadVecModelFromBytes(bytes);
    if(result.model) {
        result.fontResolution = ResolveVecModelFonts(*result.model, fontManager, docId);
    }
    return result;
}

VEDModelReadResult LoadVecModelFromBytes(const void* data, std::size_t size)
{
    VEDBinaryReader reader(data, size);
    if(!reader.Ok()) {
        return {nullptr, reader.Error(), {}, {}};
    }
    return LoadVecModelFromBytes(reader.ReadBytes(size));
}

VEDModelReadResult LoadVecModelFromBytes(const void* data, std::size_t size, TDFontManager& fontManager, TDDocumentID docId)
{
    VEDBinaryReader reader(data, size);
    if(!reader.Ok()) {
        return {nullptr, reader.Error(), {}, {}};
    }
    return LoadVecModelFromBytes(reader.ReadBytes(size), fontManager, docId);
}
