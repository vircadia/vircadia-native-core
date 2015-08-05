//
//  Gzip.h
//  libraries/shared/src
//
//  Created by Seth Alves on 2015-08-03.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef GZIP_H
#define GZIP_H

#include <QByteArray>

// The compression level must be Z_DEFAULT_COMPRESSION (-1), or between 0 and
// 9: 1 gives best speed, 9 gives best compression, 0 gives no
// compression at all (the input data is simply copied a block at a
// time).  Z_DEFAULT_COMPRESSION requests a default compromise between
// speed and compression (currently equivalent to level 6).

bool gzip(QByteArray source, QByteArray &destination, int compressionLevel = -1); // -1 is Z_DEFAULT_COMPRESSION

bool gunzip(QByteArray source, QByteArray &destination);

#endif
