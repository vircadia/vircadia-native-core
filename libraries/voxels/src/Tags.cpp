//
//  Tags.cpp
//  libraries/voxels/src
//
//  Created by Clement Brisset on 7/3/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>

#include <zconf.h>
#include <zlib.h>

#include "Tags.h"

Tag::Tag(int tagId, std::stringstream &ss) : _tagId(tagId) {
    int size = ss.get() << 8 | ss.get();

    _name.clear();
    for (int i = 0; i < size; ++i) {        _name += ss.get();
    }
}

Tag* Tag::readTag(int tagId, std::stringstream &ss) {

    switch (tagId) {
    case TAG_Byte:
        return new TagByte(ss);
    case TAG_Short:
        return new TagShort(ss);
    case TAG_Int:
        return new TagInt(ss);
    case TAG_Long:
        return new TagLong(ss);
    case TAG_Float:
        return new TagFloat(ss);
    case TAG_Double:
        return new TagDouble(ss);
    case TAG_Byte_Array:
        return new TagByteArray(ss);
    case TAG_String:
        return new TagString(ss);
    case TAG_List:
        return new TagList(ss);
    case TAG_Compound:
        return new TagCompound(ss);
    case TAG_Int_Array:
        return new TagIntArray(ss);
    default:
        return NULL;
    }
}

TagByte::TagByte(std::stringstream &ss) : Tag(TAG_Byte, ss) {
    _data = ss.get();
}

TagShort::TagShort(std::stringstream &ss) : Tag(TAG_Short, ss) {
    _data = ss.get() << 8 | ss.get();
}

TagInt::TagInt(std::stringstream &ss) : Tag(TAG_Int, ss) {
    _data = ss.get() << 24 | ss.get() << 16 | ss.get() << 8 | ss.get();
}

TagLong::TagLong(std::stringstream &ss) : Tag(TAG_Long, ss) {
    _data = (((int64_t) ss.get()) << 56 | ((int64_t) ss.get()) << 48
             |((int64_t) ss.get()) << 40 | ((int64_t) ss.get()) << 32
             |           ss.get()  << 24 |            ss.get()  << 16
             |           ss.get()  << 8  |            ss.get());
}

// We don't need Float and double, so we just ignore the bytes
TagFloat::TagFloat(std::stringstream &ss) : Tag(TAG_Float, ss) {
    ss.seekg(4, ss.cur);
}

TagDouble::TagDouble(std::stringstream &ss) : Tag(TAG_Double, ss) {
    ss.seekg(8, ss.cur);
}

TagByteArray::TagByteArray(std::stringstream &ss) : Tag(TAG_Byte_Array, ss) {
    _size = ss.get() << 24 | ss.get() << 16 | ss.get() << 8 | ss.get();

    _data = new char[_size];
    for (int i = 0; i < _size; ++i) {
        _data[i] = ss.get();
    }
}

TagString::TagString(std::stringstream &ss) : Tag(TAG_String, ss) {
    _size = ss.get() << 8 | ss.get();

    for (int i = 0; i < _size; ++i) {
        _data += ss.get();
    }
}

TagList::TagList(std::stringstream &ss) :
    Tag(TAG_List, ss) {
    _tagId = ss.get();
    _size  = ss.get() << 24 | ss.get() << 16 | ss.get() << 8 | ss.get();

    for (int i = 0; i < _size; ++i) {
        ss.putback(0);
        ss.putback(0);
        _data.push_back(readTag(_tagId, ss));
    }
}

TagList::~TagList() {
    while (!_data.empty()) {
        delete _data.back();
        _data.pop_back();
    }
}

TagCompound::TagCompound(std::stringstream &ss) :
    Tag(TAG_Compound, ss),
    _size(0),
    _width(0),
    _length(0),
    _height(0),
    _blocksData(NULL),
    _blocksId(NULL)
{
    int tagId;

    while (TAG_End != (tagId = ss.get())) {
        _data.push_back(readTag(tagId, ss));
        ++_size;

        if (NULL == _data.back()) {
            _blocksId   = NULL;
            _blocksData = NULL;
            return;
        } else if (TAG_Short == tagId) {
            if        ("Width"  == _data.back()->getName()) {
                _width  = ((TagShort*) _data.back())->getData();
            } else if ("Height" == _data.back()->getName()) {
                _height = ((TagShort*) _data.back())->getData();
            } else if ("Length" == _data.back()->getName()) {
                _length = ((TagShort*) _data.back())->getData();
            }
        } else if (TAG_Byte_Array == tagId) {
            if        ("Blocks"  == _data.back()->getName()) {
                _blocksId   = ((TagByteArray*) _data.back())->getData();
            } else if ("Data"    == _data.back()->getName()) {
                _blocksData = ((TagByteArray*) _data.back())->getData();
            }
        }
    }
}

TagCompound::~TagCompound() {
    while (!_data.empty()) {
        delete _data.back();
        _data.pop_back();
    }
}

TagIntArray::TagIntArray(std::stringstream &ss) : Tag(TAG_Int_Array, ss) {
    _size = ss.get() << 24 | ss.get() << 16 | ss.get() << 8 | ss.get();

    _data = new int[_size];
    for (int i = 0; i < _size; ++i) {
        _data[i] = ss.get();
    }
}

int retrieveData(std::string filename, std::stringstream &ss) {
    std::ifstream file(filename.c_str(), std::ios::binary);

    int type = file.peek();
    if (type == 0x0A) {
        ss.flush();
        std::copy(std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>(),
            std::ostreambuf_iterator<char>(ss));
        return 0;
    }

    if (type == 0x1F) {
        int ret = ungzip(file, ss);
        return ret;
    }

    std::cerr << "[ERROR] Schematic compression type not recognize : " << type << std::endl;
    return 1;
}

int ungzip(std::ifstream &file, std::stringstream &ss) {
    std::string gzipedBytes;
    gzipedBytes.clear();
    ss.flush();

    while (!file.eof()) {
        gzipedBytes += (char) file.get();
    }
    file.close();

    if (gzipedBytes.size() == 0) {
        ss << gzipedBytes;
        return 0;
    }

    unsigned int full_length  = gzipedBytes.size();
    unsigned int half_length  = gzipedBytes.size()/2;
    unsigned int uncompLength = full_length;

    char* uncomp = (char*) calloc(sizeof(char), uncompLength);

    z_stream strm;
    strm.next_in   = (Bytef *) gzipedBytes.c_str();
    strm.avail_in  = full_length;
    strm.total_out = 0;
    strm.zalloc    = Z_NULL;
    strm.zfree     = Z_NULL;

    bool done = false;

    if (inflateInit2(&strm, (16 + MAX_WBITS)) != Z_OK) {
        free(uncomp);
        return 1;
    }

    while (!done) {
        // If our output buffer is too small
        if (strm.total_out >= uncompLength) {
            // Increase size of output buffer
            char* uncomp2 = (char*) calloc(sizeof(char), uncompLength + half_length);
            memcpy(uncomp2, uncomp, uncompLength);
            uncompLength += half_length;
            free(uncomp);
            uncomp = uncomp2;
        }

        strm.next_out  = (Bytef *) (uncomp + strm.total_out);
        strm.avail_out = uncompLength - strm.total_out;

        // Inflate another chunk.
        int err = inflate (&strm, Z_SYNC_FLUSH);
        if (err == Z_STREAM_END) {
            done = true;
        } else if (err != Z_OK) {
            break;
        }
    }

    if (inflateEnd (&strm) != Z_OK) {
        free(uncomp);
        return 1;
    }

    for (size_t i = 0; i < strm.total_out; ++i) {
        ss << uncomp[i];
    }
    free(uncomp);

    return 0;
}


void computeBlockColor(int id, int data, int& red, int& green, int& blue, int& create) {

    switch (id) {
        case   1:
        case  14:
        case  15:
        case  16:
        case  21:
        case  56:
        case  73:
        case  74:
        case  97:
        case 129: red = 128; green = 128; blue = 128; break;
        case   2: red =  77; green = 117; blue =  66; break;
        case   3:
        case  60: red = 116; green =  83; blue =  56; break;
        case   4: red =  71; green =  71; blue =  71; break;
        case   5:
        case 125: red = 133; green =  94; blue =  62; break;
        case   7: red =  35; green =  35; blue =  35; break;
        case   8:
        case   9: red = 100; green = 109; blue = 185; break;
        case  10:
        case  11: red = 192; green =  64; blue =   8; break;
        case  12: red = 209; green = 199; blue = 155; break;
        case  13: red =  96; green =  94; blue =  93; break;
        case  17: red =  71; green =  56; blue =  35; break;
        case  18: red =  76; green = 104; blue =  64; break;
        case  19: red = 119; green = 119; blue =  37; break;
        case  22: red =  22; green =  44; blue =  86; break;
        case  23:
        case  29:
        case  33:
        case  61:
        case  62:
        case 158: red =  61; green =  61; blue =  61; break;
        case  24: red = 209; green = 202; blue = 156; break;
        case  25:
        case  58:
        case  84:
        case 137: red =  57; green =  38; blue =  25; break;
        case  35:
            switch (data) {
                case  0: red = 234; green = 234; blue = 234; break;
                case  1: red = 224; green = 140; blue =  84; break;
                case  2: red = 185; green =  90; blue = 194; break;
                case  3: red = 124; green = 152; blue = 208; break;
                case  4: red = 165; green = 154; blue =  35; break;
                case  5: red =  70; green = 187; blue =  61; break;
                case  6: red = 206; green = 124; blue = 145; break;
                case  7: red =  66; green =  66; blue =  66; break;
                case  8: red = 170; green = 176; blue = 176; break;
                case  9: red =  45; green = 108; blue =  35; break;
                case 10: red = 130; green =  62; blue =   8; break;
                case 11: red =  43; green =  51; blue =  29; break;
                case 12: red =  73; green =  47; blue =  29; break;
                case 13: red =  57; green =  76; blue =  36; break;
                case 14: red = 165; green =  58; blue =  53; break;
                case 15: red =  24; green =  24; blue =  24; break;
                default:
                    create = 0;
                    break;
            }
            break;
        case  41: red = 239; green = 238; blue = 105; break;
        case  42: red = 146; green = 146; blue = 146; break;
        case  43:
        case  98: red = 161; green = 161; blue = 161; break;
        case  44:
            create = 3;

            switch (data) {
                case 0: red = 161; green = 161; blue = 161; break;
                case 1: red = 209; green = 202; blue = 156; break;
                case 2: red = 133; green =  94; blue =  62; break;
                case 3: red =  71; green =  71; blue =  71; break;
                case 4: red = 121; green =  67; blue =  53; break;
                case 5: red = 161; green = 161; blue = 161; break;
                case 6: red =  45; green =  22; blue =  26; break;
                case 7: red = 195; green = 192; blue = 185; break;
                default:
                    create = 0;
                    break;
            }
            break;
        case  45: red = 121; green =  67; blue =  53; break;
        case  46: red = 118; green =  36; blue =  13; break;
        case  47: red = 155; green = 127; blue =  76; break;
        case  48: red =  61; green =  79; blue =  61; break;
        case  49: red =  52; green =  41; blue =  74; break;
        case  52: red =  12; green =  66; blue =  71; break;
        case  53:
        case  67:
        case 108:
        case 109:
        case 114:
        case 128:
        case 134:
        case 135:
        case 136:
        case 156:
            create = 2;

            switch (id) {
                case  53:
                case 134:
                case 135:
                case 136: red = 133; green =  94; blue =  62; break;
                case  67: red =  71; green =  71; blue =  71; break;
                case 108: red = 121; green =  67; blue =  53; break;
                case 109: red = 161; green = 161; blue = 161; break;
                case 114: red =  45; green =  22; blue =  26; break;
                case 128: red = 209; green = 202; blue = 156; break;
                case 156: red = 195; green = 192; blue = 185; break;
                default:
                    create = 0;
                    break;
            }
            break;
        case  54:
        case  95:
        case 146: red = 155; green = 105; blue =  32; break;
        case  57: red = 145; green = 219; blue = 215; break;
        case  79: red = 142; green = 162; blue = 195; break;
        case  80: red = 255; green = 255; blue = 255; break;
        case  81: red =   8; green =  64; blue =  15; break;
        case  82: red = 150; green = 155; blue = 166; break;
        case  86:
        case  91: red = 179; green = 108; blue =  17; break;
        case  87:
        case 153: red =  91; green =  31; blue =  30; break;
        case  88: red =  68; green =  49; blue =  38; break;
        case  89: red = 180; green = 134; blue =  65; break;
        case 103: red = 141; green = 143; blue =  36; break;
        case 110: red = 103; green =  92; blue =  95; break;
        case 112: red =  45; green =  22; blue =  26; break;
        case 121: red = 183; green = 178; blue = 129; break;
        case 123: red = 101; green =  59; blue =  31; break;
        case 124: red = 213; green = 178; blue = 123; break;
        case 130: red =  38; green =  54; blue =  56; break;
        case 133: red =  53; green =  84; blue =  85; break;
        case 152: red = 131; green =  22; blue =   7; break;
        case 155: red = 195; green = 192; blue = 185; break;
        case 159: red = 195; green = 165; blue = 150; break;
        case 170: red = 168; green = 139; blue =  15; break;
        case 172: red = 140; green =  86; blue =  61; break;
        case 173: red =   9; green =   9; blue =   9; break;
        default:
            create = 0;
            break;
    }
}

