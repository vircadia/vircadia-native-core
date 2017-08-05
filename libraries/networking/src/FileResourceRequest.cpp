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

#include <StatTracker.h>

void FileResourceRequest::doSend() {
    auto statTracker = DependencyManager::get<StatTracker>();
    statTracker->incrementStat(STAT_FILE_REQUEST_STARTED);
    int fileSize = 0;
    QString filename = _url.toLocalFile();
    
    // sometimes on windows, we see the toLocalFile() return null,
    // in this case we will attempt to simply use the url as a string
    if (filename.isEmpty()) {
        filename = _url.toString();
    }

    if (!_byteRange.isValid()) {
        _result = ResourceRequest::InvalidByteRange;
    } else {
        QFile file(filename);
        if (file.exists()) {
            if (file.open(QFile::ReadOnly)) {

                if (file.size() < _byteRange.fromInclusive || file.size() < _byteRange.toExclusive) {
                    _result = ResourceRequest::InvalidByteRange;
                } else {
                    // fix it up based on the known size of the file
                    _byteRange.fixupRange(file.size());

                    if (_byteRange.fromInclusive >= 0) {
                        // this is a positive byte range, simply skip to that part of the file and read from there
                        file.seek(_byteRange.fromInclusive);
                        _data = file.read(_byteRange.size());
                    } else {
                        // this is a negative byte range, we'll need to grab data from the end of the file first
                        file.seek(file.size() + _byteRange.fromInclusive);
                        _data = file.read(_byteRange.size());
                    }

                    _result = ResourceRequest::Success;
                    fileSize = file.size();
                }

            } else {
                _result = ResourceRequest::AccessDenied;
            }
        } else {
            _result = ResourceRequest::NotFound;
        }
    }
    
    _state = Finished;
    emit finished();

    if (_result == ResourceRequest::Success) {
        statTracker->incrementStat(STAT_FILE_REQUEST_SUCCESS);
        // Recording FILE bytes downloaded in stats
        statTracker->updateStat(STAT_FILE_RESOURCE_TOTAL_BYTES,fileSize);
    } else {
        statTracker->incrementStat(STAT_FILE_REQUEST_FAILED);
    }
}
