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
 protected:
  int         _tagId;
  std::string _name;
  
 public:
  Tag(int tagId, std::stringstream &ss);
  
  int         tagId() const {return _tagId;}
  std::string name () const {return _name; }
  
  static Tag* readTag(int tagId, std::stringstream &ss);
};

class TagByte : public Tag  {
 private:
  int8_t _data;
  
 public:
  TagByte(std::stringstream &ss);
  
  int8_t data() const {return _data;}
};

class TagShort : public Tag  {
 private:
  int16_t _data;
  
 public:
  TagShort(std::stringstream &ss);
  
  int16_t data() const {return _data;}
};

class TagInt : public Tag  {
 private:
  int32_t _data;
  
 public:
  TagInt(std::stringstream &ss);
  
  int32_t data() const {return _data;}
};

class TagLong : public Tag  {
 private:
  int64_t _data;
  
 public:
  TagLong(std::stringstream &ss);
  
  int64_t data() const {return _data;}
};

class TagByteArray : public Tag {
 private:
  int   _size;
  char* _data;
  
 public:
  TagByteArray(std::stringstream &ss);
  
  int   size() const {return _size;}
  char* data() const {return _data;}
};

class TagString : public Tag {
 private:
  int         _size;
  std::string _data;

 public:
  TagString(std::stringstream &ss);
  
  int         size() const {return _size;}
  std::string data() const {return _data;}
};

class TagList : public Tag {
 private:
  int            _tagId;
  int            _size;
  std::list<Tag*> _data;
  
 public:
  TagList(std::stringstream &ss);
  
  int             tagId() const {return _tagId;}
  int             size () const {return _size; }
  std::list<Tag*> data () const {return _data; }
};

class TagCompound : public Tag {
 private:
  int            _size;
  std::list<Tag*> _data;
  
  // Specific to schematics file
  int   _width;
  int   _length;
  int   _height;
  char* _blocksData;
  char* _blocksId;

 public:
  TagCompound(std::stringstream &ss);
  
  int             size      () const {return _size;      }
  std::list<Tag*> data      () const {return _data;      }

  int             width     () const {return _width;     }
  int             length    () const {return _length;    }
  int             height    () const {return _height;    }
  char*           blockId   () const {return _blocksId;  }
  char*           blocksData() const {return _blocksData;}
};

class TagIntArray : public Tag {
 private:
  int  _size;
  int* _data;

 public:
  TagIntArray(std::stringstream &ss);
  ~TagIntArray() {delete _data;}
  
  int  size() const {return _size;}
  int* data() const {return _data;}
};
