//
//  Created by Bradley Austin Davis 2015/11/04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FileClip.h"

#include <algorithm>

#include <QtCore/QDebug>

#include <Finally.h>

#include "../Frame.h"
#include "../Logging.h"
#include "BufferClip.h"


using namespace recording;

FileClip::FileClip(const QString& fileName) : _file(fileName) {
    auto size = _file.size();
    qDebug() << "Opening file of size: " << size;
    bool opened = _file.open(QIODevice::ReadOnly);
    if (!opened) {
        qCWarning(recordingLog) << "Unable to open file " << fileName;
        return;
    }
    auto mappedFile = _file.map(0, size, QFile::MapPrivateOption);
    init(mappedFile, size);
}


QString FileClip::getName() const {
    return _file.fileName();
}



bool FileClip::write(const QString& fileName, Clip::Pointer clip) {
    // FIXME need to move this to a different thread
    //qCDebug(recordingLog) << "Writing clip to file " << fileName << " with " << clip->frameCount() << " frames";

    if (0 == clip->frameCount()) {
        return false;
    }

    QFile outputFile(fileName);
    if (!outputFile.open(QFile::Truncate | QFile::WriteOnly)) {
        return false;
    }

    Finally closer([&] { outputFile.close(); });
    return clip->write(outputFile);
}

FileClip::~FileClip() {
    Locker lock(_mutex);
    _file.unmap(_data);
    if (_file.isOpen()) {
        _file.close();
    }
    reset();
}
