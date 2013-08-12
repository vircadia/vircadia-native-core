//
//  Menu.h
//  hifi
//
//  Created by Stephen Birarda on 8/12/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__Menu__
#define __hifi__Menu__

class Menu {
public:
    static void init();
private:
    static Menu* _instance;
};

#endif /* defined(__hifi__Menu__) */
