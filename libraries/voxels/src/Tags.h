//
//  Tags.h
//  hifi
//
//  Created by Clement Brisset on 7/3/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__Tags__
#define __hifi__Tags__

#include <cstdlib>
#include <cstring>

#include <fstream>
#include <sstream>
#include <list>

#define TAG_End        0
#define TAG_Byte       1
#define TAG_Short      2
#define TAG_Int        3
#define TAG_Long       4
#define TAG_Float      5
#define TAG_Double     6
#define TAG_Byte_Array 7
#define TAG_String     8
#define TAG_List       9
#define TAG_Compound  10
#define TAG_Int_Array 11

int retrieveData(std::string filename, std::stringstream &ss);
int ungzip(std::ifstream &file, std::stringstream &ss);

class Tag {
public:
    Tag(int tagId, std::stringstream &ss);

    int         getTagId() const {return _tagId;}
    std::string getName () const {return _name; }

    static Tag* readTag(int tagId, std::stringstream &ss);

protected:
    int         _tagId;
    std::string _name;
};

class TagByte : public Tag  {
public:
    TagByte(std::stringstream &ss);

    int8_t getData() const {return _data;}

private:
    int8_t _data;
};

class TagShort : public Tag  {
public:
    TagShort(std::stringstream &ss);

    int16_t getData() const {return _data;}

private:
    int16_t _data;
};

class TagInt : public Tag  {
public:
    TagInt(std::stringstream &ss);

    int32_t getData() const {return _data;}

private:
    int32_t _data;
};

class TagLong : public Tag  {
public:
    TagLong(std::stringstream &ss);

    int64_t getData() const {return _data;}

private:
    int64_t _data;
};

class TagFloat : public Tag  {
public:
    TagFloat(std::stringstream &ss);
};

class TagDouble : public Tag  {
public:
    TagDouble(std::stringstream &ss);
};

class TagByteArray : public Tag {
public:
    TagByteArray(std::stringstream &ss);

    int   getSize() const {return _size;}
    char* getData() const {return _data;}

private:
    int   _size;
    char* _data;
};

class TagString : public Tag {
public:
    TagString(std::stringstream &ss);

    int         getSize() const {return _size;}
    std::string getData() const {return _data;}

private:
    int         _size;
    std::string _data;
};

class TagList : public Tag {
public:
    TagList(std::stringstream &ss);

    int             getTagId() const {return _tagId;}
    int             getSize () const {return _size; }
    std::list<Tag*> getData () const {return _data; }

private:
    int             _tagId;
    int             _size;
    std::list<Tag*> _data;
};

class TagCompound : public Tag {
public:
    TagCompound(std::stringstream &ss);

    int             getSize      () const {return _size;      }
    std::list<Tag*> getData      () const {return _data;      }

    int             getWidth     () const {return _width;     }
    int             getLength    () const {return _length;    }
    int             getHeight    () const {return _height;    }
    char*           getBlocksId  () const {return _blocksId;  }
    char*           getBlocksData() const {return _blocksData;}

private:
    int             _size;
    std::list<Tag*> _data;

    // Specific to schematics file
    int   _width;
    int   _length;
    int   _height;
    char* _blocksData;
    char* _blocksId;
};

class TagIntArray : public Tag {
public:
    TagIntArray(std::stringstream &ss);
    ~TagIntArray() {delete _data;}

    int  getSize() const {return _size;}
    int* getData() const {return _data;}

private:
    int  _size;
    int* _data;
};

#endif /* defined(__hifi__Tags__) */
