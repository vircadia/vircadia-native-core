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

public slots:
    virtual void bake() override;

private :
    QUrl _jsURL;
    QString _bakedOutputDir;
    QString _bakedJSFilePath;

    static bool bakeJS(const QByteArray*, QByteArray*);

    static void handleSingleLineComments(QTextStream*);
    static bool handleMultiLineComments(QTextStream*);

    static bool canOmitSpace(QChar, QChar);
    static bool canOmitNewLine(QChar, QChar);

    static bool isAlphanum(QChar);
    static bool isNonAscii(QChar c);
    static bool isSpecialCharacter(QChar c);
    static bool isSpecialCharacterPrevious(QChar c);
    static bool isSpecialCharacterNext(QChar c);
    static bool isSpaceOrTab(QChar);
    static bool isQuote(QChar);
};

#endif // !hifi_JSBaker_h
