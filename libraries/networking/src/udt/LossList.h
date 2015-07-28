//
//  LossList.h
//
//
//  Created by Clement on 7/27/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LossList_h
#define hifi_LossList_h

#include <list>

#include "SeqNum.h"

namespace udt {

class LossList {
public:
    LossList() {}
    
    // Should always add at the end
    void append(SeqNum seq);
    void append(SeqNum start, SeqNum end);
    
    void remove(SeqNum seq);
    void remove(SeqNum start, SeqNum end);
    
    int getLength() const { return _length; }
    SeqNum getFirstSeqNum();
    
private:
    std::list<std::pair<SeqNum, SeqNum>> _lossList;
    int _length { 0 };
};
    
}

#endif // hifi_LossList_h