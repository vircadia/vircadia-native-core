//
//  FileResourceRequest.cpp
//  libraries/networking/src
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
    if (file.exists()) {
        if (file.open(QFile::ReadOnly)) {
            _data = file.readAll();
            _result = ResourceRequest::Success;
        } else {
            _result = ResourceRequest::AccessDenied;
        }
    } else {
        _result = ResourceRequest::NotFound;
    }
    
    _state = Finished;
    emit finished();
}
