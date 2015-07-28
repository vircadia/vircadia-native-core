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

#include "SequenceNumber.h"

namespace udt {

class LossList {
public:
    LossList() {}
    
    // Should always add at the end
    void append(SequenceNumber seq);
    void append(SequenceNumber start, SequenceNumber end);
    
    void remove(SequenceNumber seq);
    void remove(SequenceNumber start, SequenceNumber end);
    
    int getLength() const { return _length; }
    SequenceNumber getFirstSequenceNumber() const;
    
private:
    std::list<std::pair<SequenceNumber, SequenceNumber>> _lossList;
    int _length { 0 };
};
    
}

#endif // hifi_LossList_h
