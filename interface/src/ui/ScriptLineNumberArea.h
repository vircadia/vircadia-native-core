//
//  ScriptLineNumberArea.h
//  interface/src/ui
//
//  Created by Thijs Wenker on 4/30/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptLineNumberArea_h
#define hifi_ScriptLineNumberArea_h

#include <QWidget>

class ScriptEditBox;

class ScriptLineNumberArea : public QWidget {

public:
    ScriptLineNumberArea(ScriptEditBox* scriptEditBox);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    ScriptEditBox* _scriptEditBox;
};

#endif // hifi_ScriptLineNumberArea_h
