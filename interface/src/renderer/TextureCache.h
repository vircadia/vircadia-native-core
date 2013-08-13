//
//  TextureCache.h
//  interface
//
//  Created by Andrzej Kapolka on 8/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__TextureCache__
#define __interface__TextureCache__

#include "InterfaceConfig.h"

class TextureCache {
public:
    
    TextureCache();
    ~TextureCache();
    
    GLuint getPermutationNormalTextureID();

private:
    
    GLuint _permutationNormalTextureID;
};

#endif /* defined(__interface__TextureCache__) */
