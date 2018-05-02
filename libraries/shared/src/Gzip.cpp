//
//  Gzip.cpp
//  libraries/shared/src
//
//  Created by Seth Alves on 2015-08-03.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <zlib.h>
#include "Gzip.h"

const int GZIP_WINDOWS_BIT = 31;
const int GZIP_CHUNK_SIZE = 4096;
const int DEFAULT_MEM_LEVEL = 8;

bool gunzip(QByteArray source, QByteArray &destination) {
    destination.clear();
    if (source.length() == 0) {
        return true;
    }

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;

    int status = inflateInit2(&strm, GZIP_WINDOWS_BIT);

    if (status != Z_OK) {
        return false;
    }

    char *sourceData = source.data();
    int sourceDataLength = source.length();

    for (;;) {
        int chunkSize = qMin(GZIP_CHUNK_SIZE, sourceDataLength);
        if (chunkSize <= 0) {
            break;
        }

        strm.next_in = (unsigned char*)sourceData;
        strm.avail_in = chunkSize;
        sourceData += chunkSize;
        sourceDataLength -= chunkSize;

        for (;;) {
            char out[GZIP_CHUNK_SIZE];

            strm.next_out = (unsigned char*)out;
            strm.avail_out = GZIP_CHUNK_SIZE;

            status = inflate(&strm, Z_NO_FLUSH);

            switch (status) {
                case Z_NEED_DICT:
                    status = Z_DATA_ERROR;
                    // FALLTHRU
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                case Z_STREAM_ERROR:
                    inflateEnd(&strm);
                    return false;
            }

            int available = (GZIP_CHUNK_SIZE - strm.avail_out);
            if (available > 0) {
                destination.append((char*)out, available);
            }

            if (strm.avail_out != 0) {
                break;
            }
        }

        if (status == Z_STREAM_END) {
            break;
        }
    }

    inflateEnd(&strm);
    return status == Z_STREAM_END;
}

bool gzip(QByteArray source, QByteArray &destination, int compressionLevel) {
    destination.clear();
    if (source.length() == 0) {
        return true;
    }

    int flushOrFinish = 0;
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = Z_NULL;
    strm.avail_in = 0;

    int status = deflateInit2(&strm,
                              qMax(Z_DEFAULT_COMPRESSION, qMin(9, compressionLevel)),
                              Z_DEFLATED,
                              GZIP_WINDOWS_BIT,
                              DEFAULT_MEM_LEVEL,
                              Z_DEFAULT_STRATEGY);
    if (status != Z_OK) {
        return false;
    }
    char *sourceData = source.data();
    int sourceDataLength = source.length();

    for (;;) {
        int chunkSize = qMin(GZIP_CHUNK_SIZE, sourceDataLength);
        strm.next_in = (unsigned char*)sourceData;
        strm.avail_in = chunkSize;
        sourceData += chunkSize;
        sourceDataLength -= chunkSize;

        if (sourceDataLength <= 0) {
            flushOrFinish = Z_FINISH;
        } else {
            flushOrFinish = Z_NO_FLUSH;
        }

        for (;;) {
            char out[GZIP_CHUNK_SIZE];
            strm.next_out = (unsigned char*)out;
            strm.avail_out = GZIP_CHUNK_SIZE;
            status = deflate(&strm, flushOrFinish);
            if (status == Z_STREAM_ERROR) {
                deflateEnd(&strm);
                return false;
            }
            int available = (GZIP_CHUNK_SIZE - strm.avail_out);
            if (available > 0) {
                destination.append((char*)out, available);
            }
            if (strm.avail_out != 0) {
                break;
            }
        }

        if (flushOrFinish == Z_FINISH) {
            break;
        }
    }

    deflateEnd(&strm);
    return status == Z_STREAM_END;
}
