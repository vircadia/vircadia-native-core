//
//  FileResourceRequest.cpp
//
//  Created by Ryan Huffman on 2015/07/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FileResourceRequest.h"

#include <QFile>

void FileResourceRequest::doSend() {
    QString filename = _url.toLocalFile();
    QFile file(filename);
    _state = FINISHED;
    if (file.open(QFile::ReadOnly)) {
        _data = file.readAll();
        _result = ResourceRequest::SUCCESS;
        emit finished();
    } else {
        _result = ResourceRequest::ERROR;
        emit finished();
    }
}
