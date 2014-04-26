//
//  ChatInputArea.cpp
//  interface/src/ui
//
//  Created by Ryan Huffman on 4/24/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ChatInputArea.h"

ChatInputArea::ChatInputArea(QWidget* parent) : QTextEdit(parent) {
};

void ChatInputArea::insertFromMimeData(const QMimeData* source) {
    insertPlainText(source->text());
};
