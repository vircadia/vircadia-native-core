//
//  Created by Bradley Austin Davis 2015/11/05
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Recording_Impl_FileClip_h
#define hifi_Recording_Impl_FileClip_h

#include "PointerClip.h"

#include <QtCore/QFile>

namespace recording {

class FileClip : public PointerClip {
public:
    using Pointer = std::shared_ptr<FileClip>;

    FileClip(const QString& file);
    virtual ~FileClip();

    virtual QString getName() const override;

    static bool write(const QString& filePath, Clip::Pointer clip);

private:
    QFile _file;
};

}

#endif
