//
//  ParticleTreeElement.h
//  libraries/particles/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ParticleTreeElement_h
#define hifi_ParticleTreeElement_h

//#include <vector>

#include <OctreeElement.h>
#include <QList>

#include "Particle.h"
#include "ParticleTree.h"

class ParticleTree;
class ParticleTreeElement;

class ParticleTreeUpdateArgs {
public:
    QList<Particle> _movingParticles;
};

class FindAndUpdateParticleIDArgs {
public:
    uint32_t particleID;
    uint32_t creatorTokenID;
    bool creatorTokenFound;
    bool viewedParticleFound;
    bool isViewing;
};



class ParticleTreeElement : public OctreeElement {
    friend class ParticleTree; // to allow createElement to new us...

    ParticleTreeElement(unsigned char* octalCode = NULL);

    virtual OctreeElement* createNewElement(unsigned char* octalCode = NULL);

public:
    virtual ~ParticleTreeElement();

    // type safe versions of OctreeElement methods
    ParticleTreeElement* getChildAtIndex(int index) { return (ParticleTreeElement*)OctreeElement::getChildAtIndex(index); }

    // methods you can and should override to implement your tree functionality

    /// Adds a child to the current element. Override this if there is additional child initialization your class needs.
    virtual ParticleTreeElement* addChildAtIndex(int index);

    /// Override this to implement LOD averaging on changes to the tree.
    virtual void calculateAverageFromChildren();

    /// Override this to implement LOD collapsing and identical child pruning on changes to the tree.
    virtual bool collapseChildren();

    /// Should this element be considered to have content in it. This will be used in collision and ray casting methods.
    /// By default we assume that only leaves are actual content, but some octrees may have different semantics.
    virtual bool hasContent() const { return isLeaf(); }

    /// Override this to break up large octree elements when an edit operation is performed on a smaller octree element.
    /// For example, if the octrees represent solid cubes and a delete of a smaller octree element is done then the
    /// meaningful split would be to break the larger cube into smaller cubes of the same color/texture.
    virtual void splitChildren() { }

    /// Override to indicate that this element requires a split before editing lower elements in the octree
    virtual bool requiresSplit() const { return false; }

    /// Override to serialize the state of this element. This is used for persistance and for transmission across the network.
    virtual OctreeElement::AppendState appendElementData(OctreePacketData* packetData, EncodeBitstreamParams& params) const;

    /// Override to deserialize the state of this element. This is used for loading from a persisted file or from reading
    /// from the network.
    virtual int readElementDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args);

    /// Override to indicate that the item is currently rendered in the rendering engine. By default we assume that if
    /// the element should be rendered, then your rendering engine is rendering. But some rendering engines my have cases
    /// where an element is not actually rendering all should render elements. If the isRendered() state doesn't match the
    /// shouldRender() state, the tree will remark elements as changed even in cases there the elements have not changed.
    virtual bool isRendered() const { return getShouldRender(); }
    virtual bool deleteApproved() const { return !hasParticles(); }

    virtual bool findSpherePenetration(const glm::vec3& center, float radius,
                        glm::vec3& penetration, void** penetratedObject) const;

    const QList<Particle>& getParticles() const { return *_particles; }
    QList<Particle>& getParticles() { return *_particles; }
    bool hasParticles() const { return _particles->size() > 0; }

    void update(ParticleTreeUpdateArgs& args);
    void setTree(ParticleTree* tree) { _myTree = tree; }

    bool updateParticle(const Particle& particle);
    bool updateParticle(const ParticleID& particleID, const ParticleProperties& properties);
    void updateParticleID(FindAndUpdateParticleIDArgs* args);

    const Particle* getClosestParticle(glm::vec3 position) const;

    /// finds all particles that touch a sphere
    /// \param position the center of the query sphere
    /// \param radius the radius of the query sphere
    /// \param particles[out] vector of const Particle*
    void getParticles(const glm::vec3& position, float radius, QVector<const Particle*>& foundParticles) const;

    /// finds all particles that touch a box
    /// \param box the query box
    /// \param particles[out] vector of non-const Particle*
    void getParticles(const AACube& box, QVector<Particle*>& foundParticles);

    const Particle* getParticleWithID(uint32_t id) const;

    bool removeParticleWithID(uint32_t id);

protected:
    virtual void init(unsigned char * octalCode);

    void storeParticle(const Particle& particle);

    ParticleTree* _myTree;
    QList<Particle>* _particles;
};

#endif // hifi_ParticleTreeElement_h
