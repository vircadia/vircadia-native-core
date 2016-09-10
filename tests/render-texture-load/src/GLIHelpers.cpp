//
//  Created by Bradley Austin Davis on 2016/09/03
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLIHelpers.h"

#include <QtCore/QFileInfo>
#include <QtGui/QImage>

#include <gli/texture2d.hpp>
#include <gli/convert.hpp>
#include <gli/generate_mipmaps.hpp>
#include <gpu/Texture.h>

gli::format fromQImageFormat(QImage::Format format) {
    switch (format) {
        case QImage::Format_RGB32: 
            return gli::format::FORMAT_BGRA8_UNORM_PACK8;

        case QImage::Format_ARGB32:
            return gli::format::FORMAT_BGRA8_UNORM_PACK8;

        case QImage::Format_Grayscale8:
            return gli::format::FORMAT_L8_UNORM_PACK8;

        default:
            return gli::format::FORMAT_UNDEFINED;
    }
}

QString getKtxFileName(const QString& sourceFileName) {
    QFileInfo fileInfo(sourceFileName);
    QString name = fileInfo.completeBaseName();
    QString ext = fileInfo.suffix();
    QString path = fileInfo.absolutePath();
    return path + "/" + name + ".ktx";
}

QString convertTexture(const QString& sourceFile) {
    if (sourceFile.endsWith(".ktx") || sourceFile.endsWith(".dds")) {
        return sourceFile;
    }
    QImage sourceImage(sourceFile);
    gli::texture2d workTexture(
        fromQImageFormat(sourceImage.format()), 
        gli::extent2d(sourceImage.width(), sourceImage.height()));
    auto sourceSize = sourceImage.byteCount();
    assert(sourceSize == workTexture[workTexture.base_level()].size());
    memcpy(workTexture[workTexture.base_level()].data(), sourceImage.constBits(), sourceSize);

    QString resultFile = getKtxFileName(sourceFile) ;
    gli::texture2d TextureMipmaped = gli::generate_mipmaps(workTexture, gli::FILTER_LINEAR);
    gli::save(TextureMipmaped, resultFile.toLocal8Bit().data());
    gli::texture loaded = gli::load(resultFile.toLocal8Bit().data());
    return sourceFile;
}


gpu::TexturePointer processTexture(const QString& sourceFile) {
    auto ktxFile = convertTexture(sourceFile);
    gli::texture texture = gli::load(ktxFile.toLocal8Bit().data());
    if (texture.empty()) {
        return gpu::TexturePointer();
    }
    // FIXME load the actual KTX texture
    return gpu::TexturePointer();
}
