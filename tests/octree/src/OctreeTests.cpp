//
//  OctreeTests.h
//  tests/physics/src
//
//  Created by Brad Hefta-Gaub on 06/04/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDebug>

#include <PropertyFlags.h>
#include <SharedUtil.h>

#include "OctreeTests.h"

enum ModelPropertyList {
    PROP_PAGED_PROPERTY,
    PROP_CUSTOM_PROPERTIES_INCLUDED,
    PROP_VISIBLE,
    PROP_POSITION,
    PROP_RADIUS,
    PROP_MODEL_URL,
    PROP_ROTATION,
    PROP_COLOR,
    PROP_SCRIPT,
    PROP_ANIMATION_URL,
    PROP_ANIMATION_FPS,
    PROP_ANIMATION_FRAME_INDEX,
    PROP_ANIMATION_PLAYING,
    PROP_SHOULD_BE_DELETED
};

typedef PropertyFlags<ModelPropertyList> ModelPropertyFlags;

enum ParticlePropertyList {
    PARTICLE_PROP_PAGED_PROPERTY,
    PARTICLE_PROP_CUSTOM_PROPERTIES_INCLUDED,
    PARTICLE_PROP_VISIBLE,
    PARTICLE_PROP_POSITION,
    PARTICLE_PROP_RADIUS,
    PARTICLE_PROP_MODEL_URL,
    PARTICLE_PROP_ROTATION,
    PARTICLE_PROP_COLOR,
    PARTICLE_PROP_SCRIPT,
    PARTICLE_PROP_ANIMATION_URL,
    PARTICLE_PROP_ANIMATION_FPS,
    PARTICLE_PROP_ANIMATION_FRAME_INDEX,
    PARTICLE_PROP_ANIMATION_PLAYING,
    PARTICLE_PROP_SHOULD_BE_DELETED,
    PARTICLE_PROP_VELOCITY,
    PARTICLE_PROP_GRAVITY,
    PARTICLE_PROP_DAMPING,
    PARTICLE_PROP_MASS,
    PARTICLE_PROP_LIFETIME,
    PARTICLE_PROP_PAUSE_SIMULATION,
};

typedef PropertyFlags<ParticlePropertyList> ParticlePropertyFlags;


void OctreeTests::propertyFlagsTests() {
    qDebug() << "******************************************************************************************";
    qDebug() << "OctreeTests::propertyFlagsTests()";

    {    
        qDebug() << "Test 1: ModelProperties: PROP_VISIBLE, PROP_POSITION, PROP_RADIUS, PROP_MODEL_URL, PROP_ROTATION";
        ModelPropertyFlags props;
        props.setHasProperty(PROP_VISIBLE);
        props.setHasProperty(PROP_POSITION);
        props.setHasProperty(PROP_RADIUS);
        props.setHasProperty(PROP_MODEL_URL);
        props.setHasProperty(PROP_ROTATION);
    
        QByteArray encoded = props.encode();

        qDebug() << "encoded=";
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
    }

    {    
        qDebug() << "Test 2: ParticlePropertyFlags: PROP_VISIBLE, PARTICLE_PROP_ANIMATION_URL, PARTICLE_PROP_ANIMATION_FPS, "
                    "PARTICLE_PROP_ANIMATION_FRAME_INDEX, PARTICLE_PROP_ANIMATION_PLAYING, PARTICLE_PROP_PAUSE_SIMULATION";
        ParticlePropertyFlags props2;
        props2.setHasProperty(PARTICLE_PROP_VISIBLE);
        props2.setHasProperty(PARTICLE_PROP_ANIMATION_URL);
        props2.setHasProperty(PARTICLE_PROP_ANIMATION_FPS);
        props2.setHasProperty(PARTICLE_PROP_ANIMATION_FRAME_INDEX);
        props2.setHasProperty(PARTICLE_PROP_ANIMATION_PLAYING);
        props2.setHasProperty(PARTICLE_PROP_PAUSE_SIMULATION);
    
        QByteArray encoded = props2.encode();

        qDebug() << "encoded=";
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());

        qDebug() << "Test 2b: remove flag with setHasProperty() PARTICLE_PROP_PAUSE_SIMULATION";

        props2.setHasProperty(PARTICLE_PROP_PAUSE_SIMULATION, false);
    
        encoded = props2.encode();

        qDebug() << "encoded=";
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());

    }

    {    
        qDebug() << "Test 3: ParticlePropertyFlags: using | operator";
        ParticlePropertyFlags props;

        props = ParticlePropertyFlags(PARTICLE_PROP_VISIBLE) 
                    | ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_URL)
                    | ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_FPS)
                    | ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_FRAME_INDEX)
                    | ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_PLAYING) 
                    | ParticlePropertyFlags(PARTICLE_PROP_PAUSE_SIMULATION);
    
        QByteArray encoded = props.encode();

        qDebug() << "encoded=";
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());

        qDebug() << "Test 3b: remove flag with -= PARTICLE_PROP_PAUSE_SIMULATION";
        props -= PARTICLE_PROP_PAUSE_SIMULATION;
    
        encoded = props.encode();
    
        qDebug() << "encoded=";
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());

    }


    
    qDebug() << "******************************************************************************************";
}

void OctreeTests::runAllTests() {
    propertyFlagsTests();
}
