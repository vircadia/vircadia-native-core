//
//  JSBaker.h
//  libraries/baking/src
//
//  Created by Utkarsh Gautam on 9/18/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_JSBaker_h
#define hifi_JSBaker_h

#include "Baker.h"
#include "JSBakingLoggingCategory.h"

static const QString BAKED_JS_EXTENSION = ".baked.js";

class JSBaker : public Baker {
    Q_OBJECT
public:
    JSBaker(const QUrl& jsURL, const QString& bakedOutputDir);
    static bool bakeJS(const QByteArray& inputFile, QByteArray& outputFile);

public slots:
    virtual void bake() override;

private:
    QUrl _jsURL;
    QString _bakedOutputDir;
    QString _bakedJSFilePath;

    static void handleSingleLineComments(QTextStream& in);
    static bool handleMultiLineComments(QTextStream& in);

    static bool canOmitSpace(QChar previousCharacter, QChar nextCharacter);
    static bool canOmitNewLine(QChar previousCharacter, QChar nextCharacter);

    static bool isAlphanum(QChar c);
    static bool isNonAscii(QChar c);
    static bool isSpecialCharacter(QChar c);
    static bool isSpecialCharacterPrevious(QChar c);
    static bool isSpecialCharacterNext(QChar c);
    static bool isSpaceOrTab(QChar c);
    static bool isQuote(QChar c);
};

#endif // !hifi_JSBaker_h
