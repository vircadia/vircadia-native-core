//
//  Stars.h
//  interface/src
//
//  Created by Tobias Schwinger on 3/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Stars_h
#define hifi_Stars_h

class RenderArgs;

// Starfield rendering component. 
class Stars  {
public:
    Stars();
    ~Stars();

    // Renders the starfield from a local viewer's perspective.
    // The parameters specifiy the field of view.
    void render(RenderArgs* args, float alpha);
private:
    // don't copy/assign
    Stars(Stars const&); // = delete;
    Stars& operator=(Stars const&); // delete;
};


#endif // hifi_Stars_h
