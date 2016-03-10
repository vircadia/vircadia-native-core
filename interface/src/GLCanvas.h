//
//  GLCanvas.h
//  interface/src
//
//  Created by Stephen Birarda on 8/14/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GLCanvas_h
#define hifi_GLCanvas_h

#include <gl/GLWidget.h>

/// customized canvas that simply forwards requests/events to the singleton application
class GLCanvas : public GLWidget {
    Q_OBJECT
protected:
    virtual bool event(QEvent* event) override;
};


#endif // hifi_GLCanvas_h
