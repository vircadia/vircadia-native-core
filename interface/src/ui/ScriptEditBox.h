//
//  ScriptEditBox.h
//  interface/src/ui
//
//  Created by Thijs Wenker on 4/30/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptEditBox_h
#define hifi_ScriptEditBox_h

#include <QPlainTextEdit>

class ScriptEditBox : public QPlainTextEdit {
    Q_OBJECT

public:
    ScriptEditBox(QWidget* parent = NULL);

    void lineNumberAreaPaintEvent(QPaintEvent* event);
    int lineNumberAreaWidth();

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void updateLineNumberAreaWidth(int blockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect& rect, int deltaY);

private:
    QWidget* _scriptLineNumberArea;
};

#endif // hifi_ScriptEditBox_h
