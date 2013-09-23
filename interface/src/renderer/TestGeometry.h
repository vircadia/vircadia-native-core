//
//  TestGeometry.h
//  interface
//
//  Created by Andrzej Kapolka on 8/7/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__TestGeometry__
#define __interface__TestGeometry__

#include <QObject>
#include <QStack>

class QOpenGLFramebufferObject;

class ProgramObject;

/// A generic full screen glow effect.
class TestGeometry : public QObject {
    Q_OBJECT
    
public:
    TestGeometry();
    ~TestGeometry();
    
    void init();
    
    /// Starts using the geometry shader effect.
    void begin();
    
    /// Stops using the geometry shader effect.
    void end();
    
public slots:
    
private:
    
    bool _initialized;

    ProgramObject* _testProgram;
};

#endif /* defined(__interface__TestGeometry__) */
