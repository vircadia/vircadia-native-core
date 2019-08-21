//
//  TextOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TextOverlay.h"

#include <QQuickItem>
#include <QFontMetrics>

#include "FontFamilies.h"

QString const TextOverlay::TYPE = "text";
QUrl const TextOverlay::URL(QString("hifi/overlays/TextOverlay.qml"));

TextOverlay::TextOverlay() : QmlOverlay(URL) { }

TextOverlay::TextOverlay(const TextOverlay* textOverlay) 
    : QmlOverlay(URL, textOverlay) {
}

TextOverlay* TextOverlay::createClone() const {
    return new TextOverlay(this);
}

QSizeF TextOverlay::textSize(const QString& text) const {
    int lines = 1;
    foreach(QChar c, text) {
        if (c == QChar('\n')) {
            ++lines;
        }
    }
    QFont font(ROBOTO_FONT_FAMILY);
    font.setPixelSize(18);
    QFontMetrics fm(font);
    QSizeF result = QSizeF(fm.width(text), 18 * lines);
    return result; 
}