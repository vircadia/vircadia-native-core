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

class ChatEntry {
public:

    const std::string& getContents () const { return contents; }
    
    void clear ();
    
    bool key(unsigned char k);
    void specialKey(unsigned char k);
    
    void render(int screenWidth, int screenHeight);
    
private:

    std::string contents;
    
    int cursorPos;
};

#endif /* defined(__interface__ChatEntry__) */
