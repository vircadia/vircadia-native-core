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

#include <cstdlib>

#include <QFile>

void FileResourceRequest::doSend() {
    QString filename = _url.toLocalFile();
    
    // sometimes on windows, we see the toLocalFile() return null,
    // in this case we will attempt to simply use the url as a string
    if (filename.isEmpty()) {
        filename = _url.toString();
    }
    
    QFile file(filename);
    if (file.exists()) {
        if (file.open(QFile::ReadOnly)) {

            if (!_byteRange.isSet()) {
                // no byte range, read the whole file
                _data = file.readAll();
            } else {
                if (file.size() < std::abs(_byteRange.fromInclusive) || file.size() < _byteRange.toExclusive) {
                    _result = ResourceRequest::InvalidByteRange;
                } else {
                    // we have a byte range to handle
                    if (_byteRange.fromInclusive > 0) {
                        // this is a positive byte range, simply skip to that part of the file and read from there
                        file.seek(_byteRange.fromInclusive);
                        _data = file.read(_byteRange.size());
                    } else {
                        // this is a negative byte range, we'll need to grab data from the end of the file first
                        file.seek(file.size() + _byteRange.fromInclusive);
                        _data = file.read(-_byteRange.fromInclusive);

                        if (_byteRange.toExclusive > 0) {
                            // there is additional data to read from the front of the file
                            // handle that now
                            file.seek(0);
                            _data.append(file.read(_byteRange.toExclusive));
                        }
                    }
                }

            }

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
