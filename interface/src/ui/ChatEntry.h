//
//  ChatEntry.h
//  interface
//
//  Created by Andrzej Kapolka on 4/24/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__ChatEntry__
#define __interface__ChatEntry__

#include <string>

class QKeyEvent;

class ChatEntry {
public:

    const std::string& getContents() const { return _contents; }
    
    void clear();
    
    bool keyPressEvent(QKeyEvent* event);
    
    void render(int screenWidth, int screenHeight);
    
private:

    std::string _contents;
    int _cursorPos;
};

#endif /* defined(__interface__ChatEntry__) */
