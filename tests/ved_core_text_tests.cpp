#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>

#include "ved_binary_reader.h"
#include "ved_binary_writer.h"
#include "vec_edit_cad.h"
#include "vec_model.h"
#include "vec_text.h"
#include "voc_vecframetext_empty.h"
#include "voc_vectext.h"
#include "vom_vectext.h"
#include "vop_manager.h"

namespace {

std::unique_ptr<TDVecFont> loadDefaultFont()
{
    std::ifstream file("src/app/resources/font/wps_default.vfn", std::ios::binary);
    assert(file);
    std::vector<char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::unique_ptr<TDVecFont> font = LoadVecFontFromMemory(data.data(), static_cast<long>(data.size()));
    assert(font);
    return font;
}

} // namespace

int main()
{
    std::unique_ptr<TDVecFont> font = loadDefaultFont();
    SetDefaultVecTextFont(font.get());

    TDVecText text;
    text.SetVecFontPointer(font.get());
    text.SetFontName(font->GetFontName());
    text.SetOriginPoint({10.0, 20.0});
    text.SetText("AB");
    text.Initialize();

    assert(text.GetType() == VECOBJ_TEX);
    assert(text.CountGlyphs() == 2);
    assert(text.GetFrame().P3.x > text.GetFrame().P1.x);
    assert(text.GetFrame().P3.y > text.GetFrame().P1.y);

    {
        TDVecText unicodeText;
        unicodeText.SetVecFontPointer(font.get());
        unicodeText.SetFontName(font->GetFontName());
        unicodeText.SetOriginPoint({0.0, 0.0});
        const char* persianText = "\xD8\xB3" "\xD9\x84" "\xD8\xA7" "\xD9\x85";
        unicodeText.SetText(persianText);
        unicodeText.Initialize();

        assert(std::string(unicodeText.GetText()) == persianText);
        assert(unicodeText.CountGlyphs() == 4);
        assert(unicodeText.GetFrame().P3.x > unicodeText.GetFrame().P1.x);
        assert(unicodeText.GetFrame().P3.y > unicodeText.GetFrame().P1.y);
    }

    const TDMatPoint before = text.GetOriginPoint();
    assert(text.MoveBy(5.0, -3.0));
    assert(text.GetOriginPoint().x == before.x + 5.0);
    assert(text.GetOriginPoint().y == before.y - 3.0);

    text.SetSelect(true);
    const TDMatRect frameBeforeNodeMove = text.GetFrame();
    assert(text.PointOnNode(frameBeforeNodeMove.P1, 0.1) == 0);
    assert(text.PointOnNode(frameBeforeNodeMove.P3, 0.1) == 4);
    assert(text.PointOnNode({text.GetOriginPoint().x + 100000.0, text.GetOriginPoint().y + 100000.0}, 0.1) == -1);
    assert(text.MoveNode(4, 100.0, 40.0, frameBeforeNodeMove.P3));
    const TDMatRect frameAfterNodeMove = text.GetFrame();
    assert(frameAfterNodeMove.P3.x > frameBeforeNodeMove.P3.x);
    assert(frameAfterNodeMove.P3.y > frameBeforeNodeMove.P3.y);
    text.SetSelect(false);

    text.SetLockPosition(true);
    assert(!text.MoveBy(1.0, 1.0));
    text.SetLockPosition(false);

    VEDBinaryWriter writer;
    {
        writer.WriteFourCC(text.TypeFourCC());
        text.WriteTo(writer);
        assert(writer.Size() > 0);
    }

    {
        VEDBinaryReader reader(writer.Bytes());
        assert(reader.ReadFourCC() == TDVecText::StreamFourCC());
        std::unique_ptr<TDVecText> readText = TDVecText::ReadFrom(reader);
        assert(readText);
        assert(readText->GetType() == VECOBJ_TEX);
        assert(std::string(readText->GetText()) == "AB");
        assert(readText->CountGlyphs() == 2);
        assert(reader.Ok());
    }

    TDVecFrameText frameText;
    frameText.SetVecFontPointer(font.get());
    frameText.SetFontName(font->GetFontName());
    TDMatRect rect;
    rect.P1 = {0.0, 0.0};
    rect.P2 = {3000.0, 0.0};
    rect.P3 = {3000.0, 1200.0};
    rect.P4 = {0.0, 1200.0};
    frameText.SetRectangle(&rect);
    frameText.SetText("Frame");
    frameText.Initialize();

    assert(frameText.GetType() == VECOBJ_FRAMETEXT);
    assert(frameText.CountGlyphs() == 5);
    assert(frameText.GetRectangle().P3.x == 3000.0);
    assert(frameText.GetFrame().P3.x >= frameText.GetRectangle().P3.x);

    VEDBinaryWriter frameWriter;
    {
        frameWriter.WriteFourCC(frameText.TypeFourCC());
        frameText.WriteTo(frameWriter);
        assert(frameWriter.Size() > 0);
    }

    {
        VEDBinaryReader reader(frameWriter.Bytes());
        assert(reader.ReadFourCC() == TDVecFrameText::StreamFourCC());
        std::unique_ptr<TDVecFrameText> readFrameText = TDVecFrameText::ReadFrom(reader);
        assert(readFrameText);
        assert(readFrameText->GetType() == VECOBJ_FRAMETEXT);
        assert(std::string(readFrameText->GetText()) == "Frame");
        assert(readFrameText->CountGlyphs() == 5);
        assert(readFrameText->GetRectangle().P3.x == 3000.0);
        assert(reader.Ok());
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOC_VECTEXT);

        auto* extVar = dynamic_cast<TDVOCVecTextExtVar*>(operationManager.GetActiveVOPExtVariables());
        assert(extVar);

        TDVecText::TDVecTextParameter params;
        params.mpVecFont = font.get();
        std::snprintf(params.msFontName, sizeof(params.msFontName), "%s", font->GetFontName());
        extVar->SetParameter(&params);
        extVar->SetText("VOC");
        extVar->SendUpdateToParrentOP();

        operationManager.MouseMove(VIRTMOUSEBTM_UNKNOWN, VKState_KEY_UNKNOWN, 100.0, 200.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 100.0, 200.0);

        assert(model.CountObjects() == 1);
        auto* createdText = dynamic_cast<TDVecText*>(model.GetObject(0));
        assert(createdText);
        assert(createdText->GetType() == VECOBJ_TEX);
        assert(std::string(createdText->GetText()) == "VOC");
        assert(createdText->GetOriginPoint().x == 100.0);
        assert(createdText->GetOriginPoint().y == 200.0);
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);
        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOC_VECFRAMETEXT_EMPTY);

        auto* extVar = dynamic_cast<TDVOCVecFrameTextEmptyExtVar*>(operationManager.GetActiveVOPExtVariables());
        assert(extVar);

        TDVecText::TDVecTextParameter params;
        params.mpVecFont = font.get();
        std::snprintf(params.msFontName, sizeof(params.msFontName), "%s", font->GetFontName());
        extVar->SetParameter(&params);
        extVar->SendUpdateToParrentOP();

        operationManager.MouseDown(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 10.0, 20.0);
        operationManager.MouseMove(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 110.0, 70.0);
        operationManager.MouseUp(VIRTMOUSEBTM_LEFT, VKState_KEY_UNKNOWN, 110.0, 70.0);

        assert(model.CountObjects() == 1);
        auto* createdFrameText = dynamic_cast<TDVecFrameText*>(model.GetObject(0));
        assert(createdFrameText);
        assert(createdFrameText->GetType() == VECOBJ_FRAMETEXT);
        assert(createdFrameText->GetRectangle().P1.x == 10.0);
        assert(createdFrameText->GetRectangle().P1.y == 20.0);
        assert(createdFrameText->GetRectangle().P3.x == 110.0);
        assert(createdFrameText->GetRectangle().P3.y == 70.0);
    }

    {
        TDVecModel model;
        TDVecEditCad editor;
        editor.SetVecModel(&model);

        auto* textObject = new TDVecText();
        textObject->SetVecFontPointer(font.get());
        textObject->SetFontName(font->GetFontName());
        textObject->SetOriginPoint({10.0, 20.0});
        textObject->SetText("Alt");
        textObject->SetSelect(true);
        textObject->Initialize();
        model.AppendObject(textObject);

        TDViewOperationManager operationManager(&model, &editor);
        operationManager.SetOperation(VOM_VECTEXT);

        auto* extVar = dynamic_cast<TDVOMVecTextExtVar*>(operationManager.GetActiveVOPExtVariables());
        assert(extVar);
        assert(extVar->GetObjectType() == VECOBJ_TEX);
        assert(std::string(extVar->GetText()) == "Alt");

        TDVecText::TDVecTextParameter params;
        extVar->GetParameter(&params);
        params.mnXScale = 2.0;
        params.mnYScale = 1.5;
        params.mpVecFont = font.get();
        std::snprintf(params.msFontName, sizeof(params.msFontName), "%s", font->GetFontName());
        TDVecTextParameterValidity validity;
        validity.mbXScale = true;
        validity.mbYScale = true;
        validity.mbAngle = true;
        validity.mbIncline = true;
        validity.mbLineSpacing = true;
        validity.mbCharSpacing = true;
        validity.mbVertical = true;
        validity.mbUnderline = true;
        validity.mbJustification = true;
        validity.mbVerticalAlignment = true;
        validity.mbResolution = true;
        validity.mbVecFont = true;
        validity.mbScaleDependency = true;
        extVar->SetParameter(&params);
        extVar->SetParameterValidity(&validity);
        extVar->SetText("Neu");
        extVar->SendUpdateToParrentOP();

        auto* modifiedText = dynamic_cast<TDVecText*>(model.GetObject(0));
        assert(modifiedText);
        assert(std::string(modifiedText->GetText()) == "Neu");
        assert(modifiedText->GetXScale() == 2.0);
        assert(modifiedText->GetYScale() == 1.5);
    }

    SetDefaultVecTextFont(nullptr);
    return 0;
}
