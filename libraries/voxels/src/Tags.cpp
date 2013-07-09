#include "Tags.h"
#include <Log.h>

#include <zlib.h>
#include <zconf.h>

#include <iostream>

Tag::Tag(int tagId, std::stringstream &ss) : _tagId(tagId) {
    int size = ss.get() << 8 | ss.get();
  
    _name.clear();
    for (int i = 0; i < size; ++i) {
        _name += ss.get();
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
    ss.putback(0); ss.putback(0);
    _data.push_back(readTag(_tagId, ss));
  }
}

TagCompound::TagCompound(std::stringstream &ss) :
    Tag(TAG_Compound, ss),
    _size(0),
    _width(0),
    _length(0),
    _height(0),
    _blocksId(NULL),
    _blocksData(NULL)
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
  if (0x0A == type) {
    ss.flush();
    ss << file;
    return 0;
  }
  if (0x1F == type) {
    return ungzip(file, ss);
  }

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

  if ( gzipedBytes.size() == 0 ) {
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
    if (err == Z_STREAM_END) done = true;
    else if (err != Z_OK)    break;
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


