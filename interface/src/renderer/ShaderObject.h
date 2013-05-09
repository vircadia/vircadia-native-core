//
//  ShaderObject.h
//  interface
//
//  Created by Andrzej Kapolka on 5/7/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__ShaderObject__
#define __interface__ShaderObject__

#include <QObject>

#include "InterfaceConfig.h"

class ShaderObject {
public:
    
    ShaderObject(int type);
    ~ShaderObject();

    GLhandleARB getHandle() const { return _handle; }

    bool compileSourceCode(const char* data);
    bool compileSourceFile(const char* filename);

    QByteArray getLog() const;
    
private:

    Q_DISABLE_COPY(ShaderObject)
    
    GLhandleARB _handle;
};

#endif /* defined(__interface__ShaderObject__) */
