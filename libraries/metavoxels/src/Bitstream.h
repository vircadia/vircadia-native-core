//
//  Bitstream.h
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/2/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Bitstream__
#define __interface__Bitstream__

class QDataStream;

/// A stream for bit-aligned data.
class Bitstream {
public:

    Bitstream(QDataStream& underlying);

    Bitstream& write(const void* data, int bits);
    Bitstream& read(void* data, int bits);    

    /// Flushes any unwritten bits to the underlying stream.
    void flush();

    Bitstream& operator<<(bool value);
    Bitstream& operator>>(bool& value);
    
private:
   
    QDataStream& _underlying;
    char _byte;
    int _position;
};

#endif /* defined(__interface__Bitstream__) */
