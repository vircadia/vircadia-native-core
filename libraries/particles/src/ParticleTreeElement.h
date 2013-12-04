//
//  ParticleTreeElement.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__ParticleTreeElement__
#define __hifi__ParticleTreeElement__

#include <QReadWriteLock>
#include <OctreeElement.h>
#include <SharedUtil.h>

class ParticleTree;
class ParticleTreeElement;

class ParticleTreeElement : public OctreeElement {
    friend class ParticleTree; // to allow createElement to new us...
    
    ParticleTreeElement(unsigned char* octalCode = NULL);

    virtual OctreeElement* createNewElement(unsigned char* octalCode = NULL) const;
    
public:
    virtual ~ParticleTreeElement();
    virtual void init(unsigned char * octalCode);

    virtual bool hasContent() const;
    virtual void splitChildren() {}
    virtual bool requiresSplit() const { return false; }
    virtual bool appendElementData(OctreePacketData* packetData) const;
    virtual int readElementDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args);
    virtual void calculateAverageFromChildren();
    virtual bool collapseChildren();
    virtual bool isRendered() const;

    // type safe versions of OctreeElement methods
    ParticleTreeElement* getChildAtIndex(int index) { return (ParticleTreeElement*)OctreeElement::getChildAtIndex(index); }
    ParticleTreeElement* addChildAtIndex(int index) { return (ParticleTreeElement*)OctreeElement::addChildAtIndex(index); }
    
protected:
};

#endif /* defined(__hifi__ParticleTreeElement__) */