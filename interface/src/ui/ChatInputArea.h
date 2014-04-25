//
//  ChatInputArea.h
//  interface/src/ui
//
//  Created by Ryan Huffman on 4/11/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ChatInputArea_h
#define hifi_ChatInputArea_h

#include <QTextBrowser>
#include <QMimeData>

class ChatInputArea : public QTextEdit {
    Q_OBJECT
public:
    ChatInputArea(QWidget* parent);

protected:
    void insertFromMimeData(const QMimeData* source);
};

#endif // hifi_ChatInputArea_h
