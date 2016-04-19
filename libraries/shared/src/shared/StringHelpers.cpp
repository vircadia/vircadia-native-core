//
//  Created by Brad Hefta-Gaub on 2016/04/13
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QStringList>

#include "StringHelpers.h"

/// Note: this will not preserve line breaks in the original input.
QString simpleWordWrap(const QString& input, int maxCharactersPerLine) {
    QStringList words = input.split(QRegExp("\\s+"));
    QString output;
    QString currentLine;
    foreach(const auto& word, words) {
        auto newLength = currentLine.length() + word.length() + 1;

        // if this next word would fit, include it
        if (newLength <= maxCharactersPerLine) {
            currentLine += " " + word;
        } else {
            if (!output.isEmpty()) {
                output += "\n";
            }
            output += currentLine;
            currentLine = word;
        }
    }

    // if there is remaining text in the current line, append it to the end
    if (!currentLine.isEmpty()) {
        if (!output.isEmpty()) {
            output += "\n";
        }
        output += currentLine;
    }
    return output;
}
