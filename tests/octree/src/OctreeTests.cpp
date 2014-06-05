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

#include <ByteCountCoding.h>
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
        qDebug() << "Test 1: ModelProperties: using setHasProperty()";
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
        qDebug() << "Test 2: ParticlePropertyFlags: using setHasProperty()";
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

    {    
        qDebug() << "Test 3c: ParticlePropertyFlags: using |= operator";
        ParticlePropertyFlags props;

        props |= PARTICLE_PROP_VISIBLE;
        props |= PARTICLE_PROP_ANIMATION_URL;
        props |= PARTICLE_PROP_ANIMATION_FPS;
        props |= PARTICLE_PROP_ANIMATION_FRAME_INDEX;
        props |= PARTICLE_PROP_ANIMATION_PLAYING;
        props |= PARTICLE_PROP_PAUSE_SIMULATION;

        QByteArray encoded = props.encode();

        qDebug() << "encoded=";
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
    }

    {    
        qDebug() << "Test 4: ParticlePropertyFlags: using + operator";
        ParticlePropertyFlags props;

        props = ParticlePropertyFlags(PARTICLE_PROP_VISIBLE) 
                    + ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_URL)
                    + ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_FPS)
                    + ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_FRAME_INDEX)
                    + ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_PLAYING) 
                    + ParticlePropertyFlags(PARTICLE_PROP_PAUSE_SIMULATION);
    
        QByteArray encoded = props.encode();

        qDebug() << "encoded=";
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
    }

    {    
        qDebug() << "Test 4b: ParticlePropertyFlags: using += operator";
        ParticlePropertyFlags props;

        props += PARTICLE_PROP_VISIBLE;
        props += PARTICLE_PROP_ANIMATION_URL;
        props += PARTICLE_PROP_ANIMATION_FPS;
        props += PARTICLE_PROP_ANIMATION_FRAME_INDEX;
        props += PARTICLE_PROP_ANIMATION_PLAYING;
        props += PARTICLE_PROP_PAUSE_SIMULATION;
    
        QByteArray encoded = props.encode();

        qDebug() << "encoded=";
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
    }

    {    
        qDebug() << "Test 5: ParticlePropertyFlags: using = ... << operator";
        ParticlePropertyFlags props;

        props = ParticlePropertyFlags(PARTICLE_PROP_VISIBLE) 
                    << ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_URL)
                    << ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_FPS)
                    << ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_FRAME_INDEX)
                    << ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_PLAYING) 
                    << ParticlePropertyFlags(PARTICLE_PROP_PAUSE_SIMULATION);
    
        QByteArray encoded = props.encode();

        qDebug() << "encoded=";
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
    }

    {
        qDebug() << "Test 5b: ParticlePropertyFlags: using <<= operator";
        ParticlePropertyFlags props;

        props <<= PARTICLE_PROP_VISIBLE;
        props <<= PARTICLE_PROP_ANIMATION_URL;
        props <<= PARTICLE_PROP_ANIMATION_FPS;
        props <<= PARTICLE_PROP_ANIMATION_FRAME_INDEX;
        props <<= PARTICLE_PROP_ANIMATION_PLAYING;
        props <<= PARTICLE_PROP_PAUSE_SIMULATION;
    
        QByteArray encoded = props.encode();

        qDebug() << "encoded=";
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
    }

    {
        qDebug() << "Test 5c: ParticlePropertyFlags: using << enum operator";
        ParticlePropertyFlags props;

        props << PARTICLE_PROP_VISIBLE;
        props << PARTICLE_PROP_ANIMATION_URL;
        props << PARTICLE_PROP_ANIMATION_FPS;
        props << PARTICLE_PROP_ANIMATION_FRAME_INDEX;
        props << PARTICLE_PROP_ANIMATION_PLAYING;
        props << PARTICLE_PROP_PAUSE_SIMULATION;

        QByteArray encoded = props.encode();

        qDebug() << "encoded=";
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
    }

    {
        qDebug() << "Test 5d: ParticlePropertyFlags: using << flags operator ";
        ParticlePropertyFlags props;
        ParticlePropertyFlags props2;

        props << PARTICLE_PROP_VISIBLE;
        props << PARTICLE_PROP_ANIMATION_URL;
        props << PARTICLE_PROP_ANIMATION_FPS;

        props2 << PARTICLE_PROP_ANIMATION_FRAME_INDEX;
        props2 << PARTICLE_PROP_ANIMATION_PLAYING;
        props2 << PARTICLE_PROP_PAUSE_SIMULATION;

        props << props2;

        QByteArray encoded = props.encode();

        qDebug() << "encoded=";
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
    }
  
    {
        qDebug() << "Test 6: ParticlePropertyFlags comparison";
        ParticlePropertyFlags propsA;

        qDebug() << "!propsA:" << (!propsA) << "{ expect true }";

        propsA << PARTICLE_PROP_VISIBLE;
        propsA << PARTICLE_PROP_ANIMATION_URL;
        propsA << PARTICLE_PROP_ANIMATION_FPS;
        propsA << PARTICLE_PROP_ANIMATION_FRAME_INDEX;
        propsA << PARTICLE_PROP_ANIMATION_PLAYING;
        propsA << PARTICLE_PROP_PAUSE_SIMULATION;

        qDebug() << "!propsA:" << (!propsA) << "{ expect false }";

        ParticlePropertyFlags propsB;
        qDebug() << "!propsB:" << (!propsB) << "{ expect true }";


        propsB << PARTICLE_PROP_VISIBLE;
        propsB << PARTICLE_PROP_ANIMATION_URL;
        propsB << PARTICLE_PROP_ANIMATION_FPS;
        propsB << PARTICLE_PROP_ANIMATION_FRAME_INDEX;
        propsB << PARTICLE_PROP_ANIMATION_PLAYING;
        propsB << PARTICLE_PROP_PAUSE_SIMULATION;

        qDebug() << "!propsB:" << (!propsB) << "{ expect false }";

        qDebug() << "propsA == propsB:" << (propsA == propsB) << "{ expect true }";
        qDebug() << "propsA != propsB:" << (propsA != propsB) << "{ expect false }";


        qDebug() << "AFTER propsB -= PARTICLE_PROP_PAUSE_SIMULATION...";
        propsB -= PARTICLE_PROP_PAUSE_SIMULATION;

        qDebug() << "propsA == propsB:" << (propsA == propsB) << "{ expect false }";
        qDebug() << "propsA != propsB:" << (propsA != propsB) << "{ expect true }";

        qDebug() << "AFTER propsB = propsA...";
        propsB = propsA;

        qDebug() << "propsA == propsB:" << (propsA == propsB) << "{ expect true }";
        qDebug() << "propsA != propsB:" << (propsA != propsB) << "{ expect false }";

    }

    {
        qDebug() << "Test 7: ParticlePropertyFlags testing individual properties";
        ParticlePropertyFlags props;

        qDebug() << "ParticlePropertyFlags props;";
        QByteArray encoded = props.encode();
        qDebug() << "props... encoded=";
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());

        qDebug() << "props.getHasProperty(PARTICLE_PROP_VISIBLE)" << (props.getHasProperty(PARTICLE_PROP_VISIBLE)) 
                        << "{ expect false }";

        qDebug() << "props << PARTICLE_PROP_VISIBLE;";
        props << PARTICLE_PROP_VISIBLE;

        encoded = props.encode();
        qDebug() << "props... encoded=";
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
        qDebug() << "props.getHasProperty(PARTICLE_PROP_VISIBLE)" << (props.getHasProperty(PARTICLE_PROP_VISIBLE)) 
                        << "{ expect true }";

        qDebug() << "props << PARTICLE_PROP_ANIMATION_URL;";
        props << PARTICLE_PROP_ANIMATION_URL;

        encoded = props.encode();
        qDebug() << "props... encoded=";
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
        qDebug() << "props.getHasProperty(PARTICLE_PROP_VISIBLE)" << (props.getHasProperty(PARTICLE_PROP_VISIBLE)) 
                        << "{ expect true }";

        qDebug() << "props << ... more ...";
        props << PARTICLE_PROP_ANIMATION_FPS;
        props << PARTICLE_PROP_ANIMATION_FRAME_INDEX;
        props << PARTICLE_PROP_ANIMATION_PLAYING;
        props << PARTICLE_PROP_PAUSE_SIMULATION;

        encoded = props.encode();
        qDebug() << "props... encoded=";
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
        qDebug() << "props.getHasProperty(PARTICLE_PROP_VISIBLE)" << (props.getHasProperty(PARTICLE_PROP_VISIBLE)) 
                        << "{ expect true }";

        qDebug() << "ParticlePropertyFlags propsB = props & PARTICLE_PROP_VISIBLE;";
        ParticlePropertyFlags propsB = props & PARTICLE_PROP_VISIBLE;

        qDebug() << "propsB.getHasProperty(PARTICLE_PROP_VISIBLE)" << (propsB.getHasProperty(PARTICLE_PROP_VISIBLE)) 
                        << "{ expect true }";

        encoded = propsB.encode();
        qDebug() << "propsB... encoded=";
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());

        qDebug() << "ParticlePropertyFlags propsC = ~propsB;";
        ParticlePropertyFlags propsC = ~propsB;
        
        qDebug() << "propsC.getHasProperty(PARTICLE_PROP_VISIBLE)" << (propsC.getHasProperty(PARTICLE_PROP_VISIBLE))
                        << "{ expect false }";

        encoded = propsC.encode();
        qDebug() << "propsC... encoded=";
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
    }
    
    {
        qDebug() << "Test 8: ParticlePropertyFlags: decode tests";
        ParticlePropertyFlags props;

        props << PARTICLE_PROP_VISIBLE;
        props << PARTICLE_PROP_ANIMATION_URL;
        props << PARTICLE_PROP_ANIMATION_FPS;
        props << PARTICLE_PROP_ANIMATION_FRAME_INDEX;
        props << PARTICLE_PROP_ANIMATION_PLAYING;
        props << PARTICLE_PROP_PAUSE_SIMULATION;

        QByteArray encoded = props.encode();
        qDebug() << "encoded=";
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());

        qDebug() << "encoded.size()=" << encoded.size();

        ParticlePropertyFlags propsDecoded;
        propsDecoded.decode(encoded);
        
        qDebug() << "propsDecoded == props:" << (propsDecoded == props) << "{ expect true }";

        QByteArray encodedAfterDecoded = propsDecoded.encode();

        qDebug() << "encodedAfterDecoded=";
        outputBufferBits((const unsigned char*)encodedAfterDecoded.constData(), encodedAfterDecoded.size());

        qDebug() << "fill encoded byte array with extra garbage (as if it was bitstream with more content)";
        QByteArray extraContent;
        extraContent.fill(0xba, 10);
        encoded.append(extraContent);
        qDebug() << "encoded.size()=" << encoded.size() << "includes extra garbage";

        ParticlePropertyFlags propsDecodedExtra;
        propsDecodedExtra.decode(encoded);
        
        qDebug() << "propsDecodedExtra == props:" << (propsDecodedExtra == props) << "{ expect true }";

        QByteArray encodedAfterDecodedExtra = propsDecodedExtra.encode();

        qDebug() << "encodedAfterDecodedExtra=";
        outputBufferBits((const unsigned char*)encodedAfterDecodedExtra.constData(), encodedAfterDecodedExtra.size());

    }
    
    {
        qDebug() << "Test 8: ParticlePropertyFlags: QByteArray << / >> tests";
        ParticlePropertyFlags props;

        props << PARTICLE_PROP_VISIBLE;
        props << PARTICLE_PROP_ANIMATION_URL;
        props << PARTICLE_PROP_ANIMATION_FPS;
        props << PARTICLE_PROP_ANIMATION_FRAME_INDEX;
        props << PARTICLE_PROP_ANIMATION_PLAYING;
        props << PARTICLE_PROP_PAUSE_SIMULATION;

        qDebug() << "testing encoded << props";
        QByteArray encoded;
        encoded << props;
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());

        ParticlePropertyFlags propsDecoded;
        qDebug() << "testing encoded >> propsDecoded";
        encoded >> propsDecoded;
        qDebug() << "propsDecoded==props" << (propsDecoded==props);
    }

    qDebug() << "******************************************************************************************";
}


typedef ByteCountCoded<unsigned int> ByteCountCodedUINT;
typedef ByteCountCoded<quint64> ByteCountCodedQUINT64;

typedef ByteCountCoded<int> ByteCountCodedINT;

void OctreeTests::byteCountCodingTests() {
    qDebug() << "******************************************************************************************";
    qDebug() << "OctreeTests::byteCountCodingTests()";
    
    QByteArray encoded;
    
    qDebug() << "ByteCountCodedUINT zero(0)";
    ByteCountCodedUINT zero(0);
    encoded = zero.encode();
    outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());

    ByteCountCodedUINT decodedZero;
    decodedZero.decode(encoded);
    qDebug() << "decodedZero=" << decodedZero.data;
    qDebug() << "decodedZero==zero" << (decodedZero == zero) << " { expected true } ";

    ByteCountCodedUINT decodedZeroB(encoded);
    qDebug() << "decodedZeroB=" << decodedZeroB.data;

    qDebug() << "ByteCountCodedUINT foo(259)";
    ByteCountCodedUINT foo(259);
    encoded = foo.encode();
    outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());

    ByteCountCodedUINT decodedFoo;
    decodedFoo.decode(encoded);
    qDebug() << "decodedFoo=" << decodedFoo.data;
    qDebug() << "decodedFoo==foo" << (decodedFoo == foo) << " { expected true } ";

    ByteCountCodedUINT decodedFooB(encoded);
    qDebug() << "decodedFooB=" << decodedFooB.data;

    qDebug() << "ByteCountCodedUINT bar(1000000)";
    ByteCountCodedUINT bar(1000000);
    encoded = bar.encode();
    outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());

    ByteCountCodedUINT decodedBar;
    decodedBar.decode(encoded);
    qDebug() << "decodedBar=" << decodedBar.data;
    qDebug() << "decodedBar==bar" << (decodedBar == bar) << " { expected true } ";

    qDebug() << "ByteCountCodedUINT spam(4294967295/2)";
    ByteCountCodedUINT spam(4294967295/2);
    encoded = spam.encode();
    outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());

    ByteCountCodedUINT decodedSpam;
    decodedSpam.decode(encoded);
    qDebug() << "decodedSpam=" << decodedSpam.data;
    qDebug() << "decodedSpam==spam" << (decodedSpam==spam) << " { expected true } ";

    qDebug() << "ByteCountCodedQUINT64 foo64(259)";
    ByteCountCodedQUINT64 foo64(259);
    encoded = foo64.encode();
    outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
    
    qDebug() << "testing... quint64 foo64POD = foo64;";
    quint64 foo64POD = foo64;
    qDebug() << "foo64POD=" << foo64POD;

    qDebug() << "testing... encoded = foo64;";
    encoded = foo64;
    outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());

    ByteCountCodedQUINT64 decodedFoo64;
    decodedFoo64 = encoded;
    qDebug() << "decodedFoo64=" << decodedFoo64.data;
    qDebug() << "decodedFoo64==foo64" << (decodedFoo64==foo64) << " { expected true } ";

    qDebug() << "ByteCountCodedQUINT64 bar64(1000000)";
    ByteCountCodedQUINT64 bar64(1000000);
    encoded = bar64.encode();
    outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());

    ByteCountCodedQUINT64 decodedBar64;
    decodedBar64.decode(encoded);
    qDebug() << "decodedBar64=" << decodedBar64.data;
    qDebug() << "decodedBar64==bar64" << (decodedBar64==bar64) << " { expected true } ";

    qDebug() << "ByteCountCodedQUINT64 spam64(4294967295/2)";
    ByteCountCodedQUINT64 spam64(4294967295/2);
    encoded = spam64.encode();
    outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());

    ByteCountCodedQUINT64 decodedSpam64;
    decodedSpam64.decode(encoded);
    qDebug() << "decodedSpam64=" << decodedSpam64.data;
    qDebug() << "decodedSpam64==spam64" << (decodedSpam64==spam64) << " { expected true } ";

    qDebug() << "testing encoded << spam64";
    encoded.clear();
    encoded << spam64;
    outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());

    qDebug() << "testing encoded >> decodedSpam64";
    encoded >> decodedSpam64;
    qDebug() << "decodedSpam64=" << decodedSpam64.data;

    //ByteCountCodedINT shouldFail(-100);
    
    qDebug() << "NOW...";
    quint64 now = usecTimestampNow();
    ByteCountCodedQUINT64 nowCoded = now;
    QByteArray nowEncoded = nowCoded;
    outputBufferBits((const unsigned char*)nowEncoded.constData(), nowEncoded.size());
    
    
    qDebug() << "******************************************************************************************";
}

void OctreeTests::runAllTests() {
    propertyFlagsTests();
    byteCountCodingTests();
}

