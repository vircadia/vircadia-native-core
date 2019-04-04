//
//  TextureFileNamer.h
//  libraries/baking/src/baking
//
//  Created by Sabrina Shanman on 2019/03/14.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TextureFileNamer_h
#define hifi_TextureFileNamer_h

#include <QtCore/QFileInfo>
#include <QHash>

#include <image/TextureProcessing.h>

class TextureFileNamer {
public:
    TextureFileNamer() {}

    QString createBaseTextureFileName(const QFileInfo& textureFileInfo, const image::TextureUsage::Type textureType);
    
protected:
    QHash<QString, int> _textureNameMatchCount;
};

#endif // hifi_TextureFileNamer_h
