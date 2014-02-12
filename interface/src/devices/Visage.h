//
//  Visage.h
//  interface
//
//  Created by Andrzej Kapolka on 2/11/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Visage__
#define __interface__Visage__

namespace VisageSDK {
    class VisageTracker2;
}

/// Handles input from the Visage webcam feature tracking software.
class Visage {
public:
    
    Visage();
    ~Visage();
    
private:
    
    VisageSDK::VisageTracker2* _tracker;
};

#endif /* defined(__interface__Visage__) */
