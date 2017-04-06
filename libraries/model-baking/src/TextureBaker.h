//
//  TextureBaker.h
//  libraries/model-baker/src
//
//  Created by Stephen Birarda on 4/5/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TextureBaker_h
#define hifi_TextureBaker_h

#include <QtCore/QObject>
#include <QtCore/QUrl>

class TextureBaker : public QObject {
    Q_OBJECT

public:
    TextureBaker(const QUrl& textureURL);

    void start();

    const QByteArray& getOriginalTexture() const { return _originalTexture; }

    const QUrl& getTextureURL() const { return _textureURL; }

signals:
    void finished();

private slots:
    void handleTextureNetworkReply();

private:
    void bake();

    QUrl _textureURL;
    QByteArray _originalTexture;
};

#endif // hifi_TextureBaker_h
