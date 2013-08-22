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
        AudioMixer
    };
    
    Assignment(Assignment::Type type);
    Assignment(Assignment::Type type, sockaddr_in& senderSocket);
    
    Assignment::Type getType() const { return _type; }
    
private:
    Assignment::Type _type;
    sockaddr_in _senderSocket;
};



#endif /* defined(__hifi__Assignment__) */
