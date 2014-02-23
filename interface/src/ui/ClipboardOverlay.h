//
//  ClipboardOverlay.h
//  hifi
//
//  Created by Clément Brisset on 2/20/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__ClipboardOverlay__
#define __interface__ClipboardOverlay__

#include "Volume3DOverlay.h"

class ClipboardOverlay : public Volume3DOverlay {
    Q_OBJECT
    
public:
    ClipboardOverlay();
    ~ClipboardOverlay();
    
    virtual void render();
};


#endif /* defined(__interface__ClipboardOverlay__) */
