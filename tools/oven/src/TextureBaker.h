//
//  TextureBaker.h
//  tools/oven/src
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
#include <QtCore/QRunnable>

#include <image/Image.h>

#include "Baker.h"

extern const QString BAKED_TEXTURE_EXT;

class TextureBaker : public Baker {
    Q_OBJECT

public:
    TextureBaker(const QUrl& textureURL, image::TextureUsage::Type textureType, const QDir& outputDirectory);

    const QByteArray& getOriginalTexture() const { return _originalTexture; }

    QUrl getTextureURL() const { return _textureURL; }

    QString getDestinationFilePath() const { return _outputDirectory.absoluteFilePath(_bakedTextureFileName); }
    QString getBakedTextureFileName() const { return _bakedTextureFileName; }

public slots:
    virtual void bake() override;

signals:
    void originalTextureLoaded();

private slots:
    void processTexture();

private:
    void loadTexture();
    void handleTextureNetworkReply();

    QUrl _textureURL;
    QByteArray _originalTexture;
    image::TextureUsage::Type _textureType;

    QDir _outputDirectory;
    QString _bakedTextureFileName;
};

#endif // hifi_TextureBaker_h
