#include "vec_edit.h"

TDVecEdit::TDVecEdit()
    : mpVecModel(nullptr) {
}

TDVecEdit::~TDVecEdit() = default;

bool TDVecEdit::SetVecModel(TDVecModel* pVecModel) {
    mpVecModel = pVecModel;
    return mpVecModel != nullptr;
}

TDMatPoint TDVecEdit::GetTopLeftArea() {
    return mpVecModel ? mpVecModel->GetTopLeftArea() : TDMatPoint{};
}

TDMatPoint TDVecEdit::GetBottomRightArea() {
    return mpVecModel ? mpVecModel->GetBottomRightArea() : TDMatPoint{};
}
