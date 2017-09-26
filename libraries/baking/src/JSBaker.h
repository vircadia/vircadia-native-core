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

    void importJS();
    void bakeJS(QFile*);
    void exportJS(QFile*);

    void handleSingleLineComments(QTextStream*);
    void handleMultiLineComments(QTextStream*);

    bool canOmitSpace(QChar, QChar);
    bool canOmitNewLine(QChar, QChar);

    bool isAlphanum(QChar);
    bool isNonAscii(QChar c);
    bool isSpecialCharacter(QChar c);
    bool isSpecialCharacterPrevious(QChar c);
    bool isSpecialCharacterNext(QChar c);
    bool isControlCharacter(QChar);
    bool isQuote(QChar);
};

#endif // !hifi_JSBaker_h
