//
//  TextOverlay.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TextOverlay_h
#define hifi_TextOverlay_h

#include "QmlOverlay.h"

class TextOverlay : public QmlOverlay {
public:
    static QString const TYPE;
    QString getType() const override { return TYPE; }
    static QUrl const URL;


    TextOverlay();
    TextOverlay(const TextOverlay* textOverlay);
    ~TextOverlay();

    void setTopMargin(float margin);
    void setLeftMargin(float margin);
    void setFontSize(float size);
    void setText(const QString& text);


    TextOverlay* createClone() const override;
    QSizeF textSize(const QString& text) const;  // Pixels
};

 
#endif // hifi_TextOverlay_h
