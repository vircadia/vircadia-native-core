//
//  ScriptEditBox.cpp
//  interface/src/ui
//
//  Created by Thijs Wenker on 4/30/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptEditBox.h"

#include <QPainter>
#include <QTextBlock>

#include "ScriptLineNumberArea.h"

ScriptEditBox::ScriptEditBox(QWidget* parent) :
    QPlainTextEdit(parent)
{
    _scriptLineNumberArea = new ScriptLineNumberArea(this);

    connect(this, &ScriptEditBox::blockCountChanged, this, &ScriptEditBox::updateLineNumberAreaWidth);
    connect(this, &ScriptEditBox::updateRequest, this, &ScriptEditBox::updateLineNumberArea);
    connect(this, &ScriptEditBox::cursorPositionChanged, this, &ScriptEditBox::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int ScriptEditBox::lineNumberAreaWidth() {
    int digits = 1;
    const int SPACER_PIXELS = 3;
    const int BASE_TEN = 10;
    int max = qMax(1, blockCount());
    while (max >= BASE_TEN) {
        max /= BASE_TEN;
        digits++;
    }
    return SPACER_PIXELS + fontMetrics().width(QLatin1Char('H')) * digits;
}

void ScriptEditBox::updateLineNumberAreaWidth(int blockCount) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void ScriptEditBox::updateLineNumberArea(const QRect& rect, int deltaY) {
    if (deltaY) {
        _scriptLineNumberArea->scroll(0, deltaY);
    } else {
        _scriptLineNumberArea->update(0, rect.y(), _scriptLineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

void ScriptEditBox::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);

    QRect localContentsRect = contentsRect();
    _scriptLineNumberArea->setGeometry(QRect(localContentsRect.left(), localContentsRect.top(), lineNumberAreaWidth(),
                                             localContentsRect.height()));
}

void ScriptEditBox::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::gray).lighter();

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void ScriptEditBox::lineNumberAreaPaintEvent(QPaintEvent* event)
{
    QPainter painter(_scriptLineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QFont font = painter.font();
            font.setBold(this->textCursor().blockNumber() == block.blockNumber());
            painter.setFont(font);
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.drawText(0, top, _scriptLineNumberArea->width(), fontMetrics().height(),
                            Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        blockNumber++;
    }
}
