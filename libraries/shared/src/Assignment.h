//
//  Assignment.h
//  hifi
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__Assignment__
#define __hifi__Assignment__

#include "NodeList.h"

class Assignment {
public:
    
    enum Type {
        AudioMixer,
        All
    };
    
    enum Direction {
        Create,
        Request
    };
    
    Assignment(Assignment::Direction direction, Assignment::Type type, const char* pool = NULL);
    
    Assignment::Direction getDirection() const { return _direction; }
    Assignment::Type getType() const { return _type; }
    const char* getPool() const { return _pool; }
    
private:
    Assignment::Direction _direction;
    Assignment::Type _type;
    const char* _pool;
};



#endif /* defined(__hifi__Assignment__) */
