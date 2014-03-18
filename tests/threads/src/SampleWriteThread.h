//
//  SampleWriteThread.h
//  tests
//
//  Created by Brad Hefta-Gaub on 2014.03.17
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__SampleWriteThread__
#define __interface__SampleWriteThread__

#include <GenericThread.h>

class SharedResource;

/// Generalized threaded processor for handling received inbound packets. 
class SampleWriteThread : public GenericThread {
    Q_OBJECT
public:
    SampleWriteThread(SharedResource* theResource);
    
protected:
    /// Implements generic processing behavior for this thread.
    virtual bool process();

private:
    SharedResource* _theResource;
};

#endif // __interface__SampleWriteThread__
