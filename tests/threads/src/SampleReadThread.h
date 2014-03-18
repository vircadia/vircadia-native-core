//
//  SampleReadThread.h
//  tests
//
//  Created by Brad Hefta-Gaub on 2014.03.17
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__SampleReadThread__
#define __interface__SampleReadThread__

#include <GenericThread.h>

class SharedResource;

/// Generalized threaded processor for handling received inbound packets. 
class SampleReadThread : public GenericThread {
    Q_OBJECT
public:
    SampleReadThread(SharedResource* theResource, int id);
    
protected:
    /// Implements generic processing behavior for this thread.
    virtual bool process();

private:
    SharedResource* _theResource;
    int _myID;
};

#endif // __interface__SampleReadThread__
