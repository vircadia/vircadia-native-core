//
//  ScriptLineNumberArea.cpp
//  interface/src/ui
//
//  Created by Thijs Wenker on 4/30/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptLineNumberArea.h"

#include "ScriptEditBox.h"

ScriptLineNumberArea::ScriptLineNumberArea(ScriptEditBox* scriptEditBox) :
    QWidget(scriptEditBox)
{
    _scriptEditBox = scriptEditBox;
}

QSize ScriptLineNumberArea::sizeHint() const {
    return QSize(_scriptEditBox->lineNumberAreaWidth(), 0);
}

void ScriptLineNumberArea::paintEvent(QPaintEvent* event) {
    _scriptEditBox->lineNumberAreaPaintEvent(event);
}
