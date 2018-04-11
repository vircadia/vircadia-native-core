//
//  OctreeElementBag.h
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 4/25/2013.
//  Copyright 2013 High Fidelity, Inc.
//
//  This class is used by the Octree:encodeTreeBitstream() functions to store elements and element data that need to be sent.
//  It's a generic bag style storage mechanism. But It has the property that you can't put the same element into the bag
//  more than once (in other words, it de-dupes automatically).
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeElementBag_h
#define hifi_OctreeElementBag_h

#include "OctreeElement.h"

class OctreeElementExtraEncodeDataBase {
public:
    OctreeElementExtraEncodeDataBase() {}
};
using OctreeElementExtraEncodeDataBasePointer = std::shared_ptr<OctreeElementExtraEncodeDataBase>;
using OctreeElementExtraEncodeData = QMap<const OctreeElement*, OctreeElementExtraEncodeDataBasePointer>;

#endif // hifi_OctreeElementBag_h
