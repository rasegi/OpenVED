#pragma once

#include "vec_model.h"

class TDVecEdit {
public:
    TDVecEdit();
    virtual ~TDVecEdit() = 0;

    bool SetVecModel(TDVecModel* pVecModel);
    TDMatPoint GetTopLeftArea();
    TDMatPoint GetBottomRightArea();

    virtual void ViewsRefresh() = 0;

protected:
    TDVecModel* mpVecModel;
};
