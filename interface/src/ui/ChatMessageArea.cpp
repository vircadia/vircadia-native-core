//
//  ChatMessageArea.cpp
//  interface/src/ui
//
//  Created by Ryan Huffman on 4/11/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"
#include "ChatMessageArea.h"
#include <QAbstractTextDocumentLayout>
#include <QWheelEvent>

ChatMessageArea::ChatMessageArea(bool useFixedHeight) : QTextBrowser(), _useFixedHeight(useFixedHeight) {
    setOpenLinks(false);

    connect(document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged,
            this, &ChatMessageArea::updateLayout);
//    connect(this, &QTextBrowser::anchorClicked,
//            Menu::getInstance(), &Menu::openUrl);
}

void ChatMessageArea::setHtml(const QString& html) {
    // Create format with updated line height
    QTextBlockFormat format;
    format.setLineHeight(CHAT_MESSAGE_LINE_HEIGHT, QTextBlockFormat::ProportionalHeight);

    // Possibly a bug in QT, the format won't take effect if `insertHtml` is used first.  Inserting a space and deleting
    // it after ensures the format is applied.
    QTextCursor cursor = textCursor();
    cursor.setBlockFormat(format);
    cursor.insertText(" ");
    cursor.insertHtml(html);
    cursor.setPosition(0);
    cursor.deleteChar();
}

void ChatMessageArea::updateLayout() {
    if (_useFixedHeight) {
        setFixedHeight(document()->size().height());
        updateGeometry();
        emit sizeChanged(size());
    }
}

void ChatMessageArea::setSize(const QSize& size) {
    setFixedHeight(size.height());
    updateGeometry();
}

void ChatMessageArea::wheelEvent(QWheelEvent* event) {
    // Capture wheel events to stop Ctrl-WheelUp/Down zooming
    event->ignore();
}
