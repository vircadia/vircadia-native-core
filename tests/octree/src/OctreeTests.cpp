//
//  OctreeTests.h
//  tests/octree/src
//
//  Created by Brad Hefta-Gaub on 06/04/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  TODO:
//    * need to add expected results and accumulation of test success/failure
//

#include <QDebug>

#include <ByteCountCoding.h>
#include <EntityItem.h>
#include <EntityTree.h>
#include <EntityTreeElement.h>
#include <Octree.h>
#include <OctreeConstants.h>
#include <PropertyFlags.h>
#include <SharedUtil.h>

#include "OctreeTests.h"

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


void OctreeTests::propertyFlagsTests(bool verbose) {
    int testsTaken = 0;
    int testsPassed = 0;
    int testsFailed = 0;

    if (verbose) {
        qDebug() << "******************************************************************************************";
    }
    
    qDebug() << "OctreeTests::propertyFlagsTests()";

    {
        if (verbose) {
            qDebug() << "Test 1: EntityProperties: using setHasProperty()";
        }
        testsTaken++;

        EntityPropertyFlags props;
        props.setHasProperty(PROP_VISIBLE);
        props.setHasProperty(PROP_POSITION);
        props.setHasProperty(PROP_RADIUS);
        props.setHasProperty(PROP_MODEL_URL);
        props.setHasProperty(PROP_ROTATION);
    
        QByteArray encoded = props.encode();

        if (verbose) {
            qDebug() << "encoded=";
            outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
        }
        
        char expectedBytes[] = { 31 };
        QByteArray expectedResult(expectedBytes, sizeof(expectedBytes)/sizeof(expectedBytes[0]));
        
        if (encoded == expectedResult) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 1: EntityProperties: using setHasProperty()";
        }

    }

    {    
        if (verbose) {
            qDebug() << "Test 2: ParticlePropertyFlags: using setHasProperty()";
        }
        testsTaken++;

        ParticlePropertyFlags props2;
        props2.setHasProperty(PARTICLE_PROP_VISIBLE);
        props2.setHasProperty(PARTICLE_PROP_ANIMATION_URL);
        props2.setHasProperty(PARTICLE_PROP_ANIMATION_FPS);
        props2.setHasProperty(PARTICLE_PROP_ANIMATION_FRAME_INDEX);
        props2.setHasProperty(PARTICLE_PROP_ANIMATION_PLAYING);
        props2.setHasProperty(PARTICLE_PROP_PAUSE_SIMULATION);
    
        QByteArray encoded = props2.encode();

        if (verbose) {
            qDebug() << "encoded=";
            outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
        }

        char expectedBytes[] = { (char)196, (char)15, (char)2 };
        QByteArray expectedResult(expectedBytes, sizeof(expectedBytes)/sizeof(expectedBytes[0]));
        
        if (encoded == expectedResult) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 2: ParticlePropertyFlags: using setHasProperty()";
        }

        
        if (verbose) {
            qDebug() << "Test 2b: remove flag with setHasProperty() PARTICLE_PROP_PAUSE_SIMULATION";
        }
        testsTaken++;

        props2.setHasProperty(PARTICLE_PROP_PAUSE_SIMULATION, false);
    
        encoded = props2.encode();

        if (verbose) {
            qDebug() << "encoded=";
            outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
        }

        char expectedBytesB[] = { (char)136, (char)30 };
        QByteArray expectedResultB(expectedBytesB, sizeof(expectedBytesB)/sizeof(expectedBytesB[0]));
        
        if (encoded == expectedResultB) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 2b: remove flag with setHasProperty() PARTICLE_PROP_PAUSE_SIMULATION";
        }
    }

    {    
        if (verbose) {
            qDebug() << "Test 3: ParticlePropertyFlags: using | operator";
        }
        testsTaken++;
        
        ParticlePropertyFlags props;

        props = ParticlePropertyFlags(PARTICLE_PROP_VISIBLE) 
                    | ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_URL)
                    | ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_FPS)
                    | ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_FRAME_INDEX)
                    | ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_PLAYING) 
                    | ParticlePropertyFlags(PARTICLE_PROP_PAUSE_SIMULATION);
    
        QByteArray encoded = props.encode();

        if (verbose) {
            qDebug() << "encoded=";
            outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
        }
        char expectedBytes[] = { (char)196, (char)15, (char)2 };
        QByteArray expectedResult(expectedBytes, sizeof(expectedBytes)/sizeof(expectedBytes[0]));
        
        if (encoded == expectedResult) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 3: ParticlePropertyFlags: using | operator";
        }


        if (verbose) {
            qDebug() << "Test 3b: remove flag with -= PARTICLE_PROP_PAUSE_SIMULATION";
        }
        testsTaken++;
        
        props -= PARTICLE_PROP_PAUSE_SIMULATION;
    
        encoded = props.encode();
    
        if (verbose) {
            qDebug() << "encoded=";
            outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
        }
        
        char expectedBytesB[] = { (char)136, (char)30 };
        QByteArray expectedResultB(expectedBytesB, sizeof(expectedBytesB)/sizeof(expectedBytesB[0]));
        
        if (encoded == expectedResultB) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 3b: remove flag with -= PARTICLE_PROP_PAUSE_SIMULATION";
        }
        
    }

    {    
        if (verbose) {
            qDebug() << "Test 3c: ParticlePropertyFlags: using |= operator";
        }
        testsTaken++;

        ParticlePropertyFlags props;

        props |= PARTICLE_PROP_VISIBLE;
        props |= PARTICLE_PROP_ANIMATION_URL;
        props |= PARTICLE_PROP_ANIMATION_FPS;
        props |= PARTICLE_PROP_ANIMATION_FRAME_INDEX;
        props |= PARTICLE_PROP_ANIMATION_PLAYING;
        props |= PARTICLE_PROP_PAUSE_SIMULATION;

        QByteArray encoded = props.encode();

        if (verbose) {
            qDebug() << "encoded=";
            outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
        }

        char expectedBytes[] = { (char)196, (char)15, (char)2 };
        QByteArray expectedResult(expectedBytes, sizeof(expectedBytes)/sizeof(expectedBytes[0]));
        
        if (encoded == expectedResult) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - 3c: ParticlePropertyFlags: using |= operator";
        }
    }

    {    
        if (verbose) {
            qDebug() << "Test 4: ParticlePropertyFlags: using + operator";
        }
        testsTaken++;

        ParticlePropertyFlags props;

        props = ParticlePropertyFlags(PARTICLE_PROP_VISIBLE) 
                    + ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_URL)
                    + ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_FPS)
                    + ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_FRAME_INDEX)
                    + ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_PLAYING) 
                    + ParticlePropertyFlags(PARTICLE_PROP_PAUSE_SIMULATION);
    
        QByteArray encoded = props.encode();

        if (verbose) {
            qDebug() << "encoded=";
            outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
        }

        char expectedBytes[] = { (char)196, (char)15, (char)2 };
        QByteArray expectedResult(expectedBytes, sizeof(expectedBytes)/sizeof(expectedBytes[0]));
        
        if (encoded == expectedResult) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 4: ParticlePropertyFlags: using + operator";
        }
    }

    {    
        if (verbose) {
            qDebug() << "Test 5: ParticlePropertyFlags: using += operator";
        }
        testsTaken++;
        ParticlePropertyFlags props;

        props += PARTICLE_PROP_VISIBLE;
        props += PARTICLE_PROP_ANIMATION_URL;
        props += PARTICLE_PROP_ANIMATION_FPS;
        props += PARTICLE_PROP_ANIMATION_FRAME_INDEX;
        props += PARTICLE_PROP_ANIMATION_PLAYING;
        props += PARTICLE_PROP_PAUSE_SIMULATION;
    
        QByteArray encoded = props.encode();

        if (verbose) {
            qDebug() << "encoded=";
            outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
        }

        char expectedBytes[] = { (char)196, (char)15, (char)2 };
        QByteArray expectedResult(expectedBytes, sizeof(expectedBytes)/sizeof(expectedBytes[0]));
        
        if (encoded == expectedResult) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 5: ParticlePropertyFlags: using += operator";
        }
    }

    {    
        if (verbose) {
            qDebug() << "Test 6: ParticlePropertyFlags: using = ... << operator";
        }
        testsTaken++;
        
        ParticlePropertyFlags props;

        props = ParticlePropertyFlags(PARTICLE_PROP_VISIBLE) 
                    << ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_URL)
                    << ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_FPS)
                    << ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_FRAME_INDEX)
                    << ParticlePropertyFlags(PARTICLE_PROP_ANIMATION_PLAYING) 
                    << ParticlePropertyFlags(PARTICLE_PROP_PAUSE_SIMULATION);
    
        QByteArray encoded = props.encode();

        if (verbose) {
            qDebug() << "encoded=";
            outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
        }

        char expectedBytes[] = { (char)196, (char)15, (char)2 };
        QByteArray expectedResult(expectedBytes, sizeof(expectedBytes)/sizeof(expectedBytes[0]));
        
        if (encoded == expectedResult) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 6: ParticlePropertyFlags: using = ... << operator";
        }
    }

    {
        if (verbose) {
            qDebug() << "Test 7: ParticlePropertyFlags: using <<= operator";
        }
        testsTaken++;
        
        ParticlePropertyFlags props;

        props <<= PARTICLE_PROP_VISIBLE;
        props <<= PARTICLE_PROP_ANIMATION_URL;
        props <<= PARTICLE_PROP_ANIMATION_FPS;
        props <<= PARTICLE_PROP_ANIMATION_FRAME_INDEX;
        props <<= PARTICLE_PROP_ANIMATION_PLAYING;
        props <<= PARTICLE_PROP_PAUSE_SIMULATION;
    
        QByteArray encoded = props.encode();

        if (verbose) {
            qDebug() << "encoded=";
            outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
        }

        char expectedBytes[] = { (char)196, (char)15, (char)2 };
        QByteArray expectedResult(expectedBytes, sizeof(expectedBytes)/sizeof(expectedBytes[0]));
        
        if (encoded == expectedResult) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 7: ParticlePropertyFlags: using <<= operator";
        }
    }

    {
        if (verbose) {
            qDebug() << "Test 8: ParticlePropertyFlags: using << enum operator";
        }
        testsTaken++;
        
        ParticlePropertyFlags props;

        props << PARTICLE_PROP_VISIBLE;
        props << PARTICLE_PROP_ANIMATION_URL;
        props << PARTICLE_PROP_ANIMATION_FPS;
        props << PARTICLE_PROP_ANIMATION_FRAME_INDEX;
        props << PARTICLE_PROP_ANIMATION_PLAYING;
        props << PARTICLE_PROP_PAUSE_SIMULATION;

        QByteArray encoded = props.encode();

        if (verbose) {
            qDebug() << "encoded=";
            outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
        }

        char expectedBytes[] = { (char)196, (char)15, (char)2 };
        QByteArray expectedResult(expectedBytes, sizeof(expectedBytes)/sizeof(expectedBytes[0]));
        
        if (encoded == expectedResult) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 8: ParticlePropertyFlags: using << enum operator";
        }
    }

    {
        if (verbose) {
            qDebug() << "Test 9: ParticlePropertyFlags: using << flags operator ";
        }
        testsTaken++;

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

        if (verbose) {
            qDebug() << "encoded=";
            outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
        }

        char expectedBytes[] = { (char)196, (char)15, (char)2 };
        QByteArray expectedResult(expectedBytes, sizeof(expectedBytes)/sizeof(expectedBytes[0]));
        
        if (encoded == expectedResult) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 9: ParticlePropertyFlags: using << flags operator";
        }
    }
  
    {
        if (verbose) {
            qDebug() << "Test 10: ParticlePropertyFlags comparison";
        }
        ParticlePropertyFlags propsA;

        if (verbose) {
            qDebug() << "!propsA:" << (!propsA) << "{ expect true }";
        }
        testsTaken++;
        bool resultA = (!propsA);
        bool expectedA = true;
        if (resultA == expectedA) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 10a: ParticlePropertyFlags comparison, uninitialized !propsA";
        }

        propsA << PARTICLE_PROP_VISIBLE;
        propsA << PARTICLE_PROP_ANIMATION_URL;
        propsA << PARTICLE_PROP_ANIMATION_FPS;
        propsA << PARTICLE_PROP_ANIMATION_FRAME_INDEX;
        propsA << PARTICLE_PROP_ANIMATION_PLAYING;
        propsA << PARTICLE_PROP_PAUSE_SIMULATION;

        if (verbose) {
            qDebug() << "!propsA:" << (!propsA) << "{ expect false }";
        }
        testsTaken++;
        bool resultB = (!propsA);
        bool expectedB = false;
        if (resultB == expectedB) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 10b: ParticlePropertyFlags comparison, initialized !propsA";
        }

        ParticlePropertyFlags propsB;
        propsB << PARTICLE_PROP_VISIBLE;
        propsB << PARTICLE_PROP_ANIMATION_URL;
        propsB << PARTICLE_PROP_ANIMATION_FPS;
        propsB << PARTICLE_PROP_ANIMATION_FRAME_INDEX;
        propsB << PARTICLE_PROP_ANIMATION_PLAYING;
        propsB << PARTICLE_PROP_PAUSE_SIMULATION;

        if (verbose) {
            qDebug() << "propsA == propsB:" << (propsA == propsB) << "{ expect true }";
            qDebug() << "propsA != propsB:" << (propsA != propsB) << "{ expect false }";
        }
        testsTaken++;
        bool resultC = (propsA == propsB);
        bool expectedC = true;
        if (resultC == expectedC) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 10c: ParticlePropertyFlags comparison, propsA == propsB";
        }

        testsTaken++;
        bool resultD = (propsA != propsB);
        bool expectedD = false;
        if (resultD == expectedD) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 10d: ParticlePropertyFlags comparison, propsA != propsB";
        }
        
        if (verbose) {
            qDebug() << "AFTER propsB -= PARTICLE_PROP_PAUSE_SIMULATION...";
        }
        
        propsB -= PARTICLE_PROP_PAUSE_SIMULATION;

        if (verbose) {
            qDebug() << "propsA == propsB:" << (propsA == propsB) << "{ expect false }";
            qDebug() << "propsA != propsB:" << (propsA != propsB) << "{ expect true }";
        }
        testsTaken++;
        bool resultE = (propsA == propsB);
        bool expectedE = false;
        if (resultE == expectedE) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 10e: ParticlePropertyFlags comparison, AFTER propsB -= PARTICLE_PROP_PAUSE_SIMULATION";
        }
        
        if (verbose) {
            qDebug() << "AFTER propsB = propsA...";
        }
        propsB = propsA;
        if (verbose) {
            qDebug() << "propsA == propsB:" << (propsA == propsB) << "{ expect true }";
            qDebug() << "propsA != propsB:" << (propsA != propsB) << "{ expect false }";
        }
        testsTaken++;
        bool resultF = (propsA == propsB);
        bool expectedF = true;
        if (resultF == expectedF) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 10f: ParticlePropertyFlags comparison, AFTER propsB = propsA";
        }
    }

    {
        if (verbose) {
            qDebug() << "Test 11: ParticlePropertyFlags testing individual properties";
        }
        ParticlePropertyFlags props;

        if (verbose) {
            qDebug() << "ParticlePropertyFlags props;";
        }
        
        QByteArray encoded = props.encode();

        if (verbose) {
            qDebug() << "props... encoded=";
            outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
        }

        char expectedBytes[] = { 0 };
        QByteArray expectedResult(expectedBytes, sizeof(expectedBytes)/sizeof(expectedBytes[0]));
        
        testsTaken++;
        if (encoded == expectedResult) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 11a: ParticlePropertyFlags testing individual properties";
        }


        if (verbose) {
            qDebug() << "Test 11b: props.getHasProperty(PARTICLE_PROP_VISIBLE)" << (props.getHasProperty(PARTICLE_PROP_VISIBLE)) 
                        << "{ expect false }";
        }
        testsTaken++;
        bool resultB = props.getHasProperty(PARTICLE_PROP_VISIBLE);
        bool expectedB = false;
        if (resultB == expectedB) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 11b: props.getHasProperty(PARTICLE_PROP_VISIBLE)";
        }

        if (verbose) {
            qDebug() << "props << PARTICLE_PROP_VISIBLE;";
        }
        props << PARTICLE_PROP_VISIBLE;
        testsTaken++;
        bool resultC = props.getHasProperty(PARTICLE_PROP_VISIBLE);
        bool expectedC = true;
        if (resultC == expectedC) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 11c: props.getHasProperty(PARTICLE_PROP_VISIBLE) after props << PARTICLE_PROP_VISIBLE";
        }

        encoded = props.encode();

        if (verbose) {
            qDebug() << "props... encoded=";
            outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
            qDebug() << "props.getHasProperty(PARTICLE_PROP_VISIBLE)" << (props.getHasProperty(PARTICLE_PROP_VISIBLE)) 
                            << "{ expect true }";
        }

        char expectedBytesC[] = { 16 };
        QByteArray expectedResultC(expectedBytesC, sizeof(expectedBytesC)/sizeof(expectedBytesC[0]));
        
        testsTaken++;
        if (encoded == expectedResultC) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 11c: ParticlePropertyFlags testing individual properties";
        }

        if (verbose) {
            qDebug() << "props << PARTICLE_PROP_ANIMATION_URL;";
        }
        props << PARTICLE_PROP_ANIMATION_URL;

        encoded = props.encode();
        if (verbose) {
            qDebug() << "props... encoded=";
            outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
            qDebug() << "props.getHasProperty(PARTICLE_PROP_VISIBLE)" << (props.getHasProperty(PARTICLE_PROP_VISIBLE)) 
                            << "{ expect true }";
        }
        char expectedBytesD[] = { (char)136, (char)16 };
        QByteArray expectedResultD(expectedBytesD, sizeof(expectedBytesD)/sizeof(expectedBytesD[0]));
        
        testsTaken++;
        if (encoded == expectedResultD) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 11d: ParticlePropertyFlags testing individual properties";
        }
        testsTaken++;
        bool resultE = props.getHasProperty(PARTICLE_PROP_VISIBLE);
        bool expectedE = true;
        if (resultE == expectedE) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 11e: props.getHasProperty(PARTICLE_PROP_VISIBLE) after props << PARTICLE_PROP_ANIMATION_URL";
        }


        if (verbose) {
            qDebug() << "props << ... more ...";
        }
        props << PARTICLE_PROP_ANIMATION_FPS;
        props << PARTICLE_PROP_ANIMATION_FRAME_INDEX;
        props << PARTICLE_PROP_ANIMATION_PLAYING;
        props << PARTICLE_PROP_PAUSE_SIMULATION;

        encoded = props.encode();
        if (verbose) {
            qDebug() << "props... encoded=";
            outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
            qDebug() << "props.getHasProperty(PARTICLE_PROP_VISIBLE)" << (props.getHasProperty(PARTICLE_PROP_VISIBLE)) 
                            << "{ expect true }";
        }
        testsTaken++;
        bool resultF = props.getHasProperty(PARTICLE_PROP_VISIBLE);
        bool expectedF = true;
        if (resultF == expectedF) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 11f: props.getHasProperty(PARTICLE_PROP_VISIBLE) after props << more";
        }

        if (verbose) {
            qDebug() << "ParticlePropertyFlags propsB = props & PARTICLE_PROP_VISIBLE;";
        }
        ParticlePropertyFlags propsB = props & PARTICLE_PROP_VISIBLE;

        if (verbose) {
            qDebug() << "propsB.getHasProperty(PARTICLE_PROP_VISIBLE)" << (propsB.getHasProperty(PARTICLE_PROP_VISIBLE)) 
                        << "{ expect true }";
        }
        testsTaken++;
        bool resultG = propsB.getHasProperty(PARTICLE_PROP_VISIBLE);
        bool expectedG = true;
        if (resultG == expectedG) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 11g: propsB = props & PARTICLE_PROP_VISIBLE";
        }

        encoded = propsB.encode();
        if (verbose) {
            qDebug() << "propsB... encoded=";
            outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
        }
        char expectedBytesH[] = { 16 };
        QByteArray expectedResultH(expectedBytesC, sizeof(expectedBytesH)/sizeof(expectedBytesH[0]));
        
        testsTaken++;
        if (encoded == expectedResultH) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 11h: ParticlePropertyFlags testing individual properties";
        }

        if (verbose) {
            qDebug() << "ParticlePropertyFlags propsC = ~propsB;";
        }
        ParticlePropertyFlags propsC = ~propsB;
        
        if (verbose) {
            qDebug() << "propsC.getHasProperty(PARTICLE_PROP_VISIBLE)" << (propsC.getHasProperty(PARTICLE_PROP_VISIBLE))
                        << "{ expect false }";
        }
        testsTaken++;
        bool resultI = propsC.getHasProperty(PARTICLE_PROP_VISIBLE);
        bool expectedI = false;
        if (resultI == expectedI) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 11i: propsC = ~propsB";
        }

        encoded = propsC.encode();
        if (verbose) {
            qDebug() << "propsC... encoded=";
            outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
        }
    }
    
    {
        if (verbose) {
            qDebug() << "Test 12: ParticlePropertyFlags: decode tests";
        }
        ParticlePropertyFlags props;

        props << PARTICLE_PROP_VISIBLE;
        props << PARTICLE_PROP_ANIMATION_URL;
        props << PARTICLE_PROP_ANIMATION_FPS;
        props << PARTICLE_PROP_ANIMATION_FRAME_INDEX;
        props << PARTICLE_PROP_ANIMATION_PLAYING;
        props << PARTICLE_PROP_PAUSE_SIMULATION;

        QByteArray encoded = props.encode();
        if (verbose) {
            qDebug() << "encoded=";
            outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
            qDebug() << "encoded.size()=" << encoded.size();
        }

        ParticlePropertyFlags propsDecoded;
        propsDecoded.decode(encoded);
        
        if (verbose) {
            qDebug() << "propsDecoded == props:" << (propsDecoded == props) << "{ expect true }";
        }
        testsTaken++;
        bool resultA = (propsDecoded == props);
        bool expectedA = true;
        if (resultA == expectedA) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 12a: propsDecoded == props";
        }

        QByteArray encodedAfterDecoded = propsDecoded.encode();

        if (verbose) {
            qDebug() << "encodedAfterDecoded=";
            outputBufferBits((const unsigned char*)encodedAfterDecoded.constData(), encodedAfterDecoded.size());
        }
        testsTaken++;
        bool resultB = (encoded == encodedAfterDecoded);
        bool expectedB = true;
        if (resultB == expectedB) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 12b: (encoded == encodedAfterDecoded)";
        }

        if (verbose) {
            qDebug() << "fill encoded byte array with extra garbage (as if it was bitstream with more content)";
        }
        QByteArray extraContent;
        extraContent.fill(0xba, 10);
        encoded.append(extraContent);

        if (verbose) {
            qDebug() << "encoded.size()=" << encoded.size() << "includes extra garbage";
        }

        ParticlePropertyFlags propsDecodedExtra;
        propsDecodedExtra.decode(encoded);
        
        if (verbose) {
            qDebug() << "propsDecodedExtra == props:" << (propsDecodedExtra == props) << "{ expect true }";
        }
        testsTaken++;
        bool resultC = (propsDecodedExtra == props);
        bool expectedC = true;
        if (resultC == expectedC) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 12c: (propsDecodedExtra == props)";
        }

        QByteArray encodedAfterDecodedExtra = propsDecodedExtra.encode();

        if (verbose) {
            qDebug() << "encodedAfterDecodedExtra=";
            outputBufferBits((const unsigned char*)encodedAfterDecodedExtra.constData(), encodedAfterDecodedExtra.size());
        }
    }
    
    {
        if (verbose) {
            qDebug() << "Test 13: ParticlePropertyFlags: QByteArray << / >> tests";
        }
        ParticlePropertyFlags props;

        props << PARTICLE_PROP_VISIBLE;
        props << PARTICLE_PROP_ANIMATION_URL;
        props << PARTICLE_PROP_ANIMATION_FPS;
        props << PARTICLE_PROP_ANIMATION_FRAME_INDEX;
        props << PARTICLE_PROP_ANIMATION_PLAYING;
        props << PARTICLE_PROP_PAUSE_SIMULATION;

        if (verbose) {
            qDebug() << "testing encoded << props";
        }
        QByteArray encoded;
        encoded << props;

        if (verbose) {
            outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
        }

        ParticlePropertyFlags propsDecoded;
        if (verbose) {
            qDebug() << "testing encoded >> propsDecoded";
        }
        encoded >> propsDecoded;

        if (verbose) {
            qDebug() << "propsDecoded==props" << (propsDecoded==props);
        }

        testsTaken++;
        bool resultA = (propsDecoded == props);
        bool expectedA = true;
        if (resultA == expectedA) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 13: ParticlePropertyFlags: QByteArray << / >> tests";
        }
    }

    qDebug() << "   tests passed:" << testsPassed << "out of" << testsTaken;
    if (verbose) {
        qDebug() << "******************************************************************************************";
    }
}


typedef ByteCountCoded<unsigned int> ByteCountCodedUINT;
typedef ByteCountCoded<quint64> ByteCountCodedQUINT64;

typedef ByteCountCoded<int> ByteCountCodedINT;

void OctreeTests::byteCountCodingTests(bool verbose) {
    int testsTaken = 0;
    int testsPassed = 0;
    int testsFailed = 0;

    if (verbose) {
        qDebug() << "******************************************************************************************";
    }
    
    qDebug() << "OctreeTests::byteCountCodingTests()";
    
    QByteArray encoded;
    
    if (verbose) {
        qDebug() << "ByteCountCodedUINT zero(0)";
    }
    ByteCountCodedUINT zero(0);
    encoded = zero.encode();
    if (verbose) {
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
    }

    ByteCountCodedUINT decodedZero;
    decodedZero.decode(encoded);
    if (verbose) {
        qDebug() << "decodedZero=" << decodedZero.data;
        qDebug() << "decodedZero==zero" << (decodedZero == zero) << " { expected true } ";
    }
    testsTaken++;
    bool result1 = (decodedZero.data == 0);
    bool expected1 = true;
    if (result1 == expected1) {
        testsPassed++;
    } else {
        testsFailed++;
        qDebug() << "FAILED - Test 1: ByteCountCodedUINT zero(0) decodedZero.data == 0";
    }

    testsTaken++;
    bool result2 = (decodedZero == zero);
    bool expected2 = true;
    if (result2 == expected2) {
        testsPassed++;
    } else {
        testsFailed++;
        qDebug() << "FAILED - Test 2: ByteCountCodedUINT zero(0) (decodedZero == zero)";
    }

    ByteCountCodedUINT decodedZeroB(encoded);
    if (verbose) {
        qDebug() << "decodedZeroB=" << decodedZeroB.data;
    }
    testsTaken++;
    bool result3 = (decodedZeroB.data == 0);
    bool expected3 = true;
    if (result3 == expected3) {
        testsPassed++;
    } else {
        testsFailed++;
        qDebug() << "FAILED - Test 3: (decodedZeroB.data == 0)";
    }

    if (verbose) {
        qDebug() << "ByteCountCodedUINT foo(259)";
    }
    ByteCountCodedUINT foo(259);
    encoded = foo.encode();

    if (verbose) {
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
    }

    ByteCountCodedUINT decodedFoo;
    decodedFoo.decode(encoded);

    if (verbose) {
        qDebug() << "decodedFoo=" << decodedFoo.data;
        qDebug() << "decodedFoo==foo" << (decodedFoo == foo) << " { expected true } ";
    }
    testsTaken++;
    bool result4 = (decodedFoo.data == 259);
    bool expected4 = true;
    if (result4 == expected4) {
        testsPassed++;
    } else {
        testsFailed++;
        qDebug() << "FAILED - Test 4: ByteCountCodedUINT zero(0) (decodedFoo.data == 259)";
    }

    testsTaken++;
    bool result5 = (decodedFoo == foo);
    bool expected5 = true;
    if (result5 == expected5) {
        testsPassed++;
    } else {
        testsFailed++;
        qDebug() << "FAILED - Test 5: (decodedFoo == foo)";
    }

    ByteCountCodedUINT decodedFooB(encoded);
    if (verbose) {
        qDebug() << "decodedFooB=" << decodedFooB.data;
    }
    testsTaken++;
    bool result6 = (decodedFooB.data == 259);
    bool expected6 = true;
    if (result6 == expected6) {
        testsPassed++;
    } else {
        testsFailed++;
        qDebug() << "FAILED - Test 6: (decodedFooB.data == 259)";
    }


    if (verbose) {
        qDebug() << "ByteCountCodedUINT bar(1000000)";
    }
    ByteCountCodedUINT bar(1000000);
    encoded = bar.encode();
    if (verbose) {
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
    }

    ByteCountCodedUINT decodedBar;
    decodedBar.decode(encoded);
    if (verbose) {
        qDebug() << "decodedBar=" << decodedBar.data;
        qDebug() << "decodedBar==bar" << (decodedBar == bar) << " { expected true } ";
    }
    testsTaken++;
    bool result7 = (decodedBar.data == 1000000);
    bool expected7 = true;
    if (result7 == expected7) {
        testsPassed++;
    } else {
        testsFailed++;
        qDebug() << "FAILED - Test 7: ByteCountCodedUINT zero(0) (decodedBar.data == 1000000)";
    }

    testsTaken++;
    bool result8 = (decodedBar == bar);
    bool expected8 = true;
    if (result8 == expected8) {
        testsPassed++;
    } else {
        testsFailed++;
        qDebug() << "FAILED - Test 8: (decodedBar == bar)";
    }

    if (verbose) {
        qDebug() << "ByteCountCodedUINT spam(4294967295/2)";
    }
    ByteCountCodedUINT spam(4294967295/2);
    encoded = spam.encode();
    if (verbose) {
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
    }

    ByteCountCodedUINT decodedSpam;
    decodedSpam.decode(encoded);
    if (verbose) {
        qDebug() << "decodedSpam=" << decodedSpam.data;
        qDebug() << "decodedSpam==spam" << (decodedSpam==spam) << " { expected true } ";
    }
    testsTaken++;
    bool result9 = (decodedSpam.data == 4294967295/2);
    bool expected9 = true;
    if (result9 == expected9) {
        testsPassed++;
    } else {
        testsFailed++;
        qDebug() << "FAILED - Test 9: (decodedSpam.data == 4294967295/2)";
    }

    testsTaken++;
    bool result10 = (decodedSpam == spam);
    bool expected10 = true;
    if (result10 == expected10) {
        testsPassed++;
    } else {
        testsFailed++;
        qDebug() << "FAILED - Test 10: (decodedSpam == spam)";
    }

    if (verbose) {
        qDebug() << "ByteCountCodedQUINT64 foo64(259)";
    }
    ByteCountCodedQUINT64 foo64(259);
    encoded = foo64.encode();
    if (verbose) {
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
    }
    
    if (verbose) {
        qDebug() << "testing... quint64 foo64POD = foo64;";
    }
    quint64 foo64POD = foo64;
    if (verbose) {
        qDebug() << "foo64POD=" << foo64POD;
    }

    testsTaken++;
    bool result11 = (foo64POD == 259);
    bool expected11 = true;
    if (result11 == expected11) {
        testsPassed++;
    } else {
        testsFailed++;
        qDebug() << "FAILED - Test 11: quint64 foo64POD = foo64";
    }

    if (verbose) {
        qDebug() << "testing... encoded = foo64;";
    }
    encoded = foo64;

    if (verbose) {
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
    }

    ByteCountCodedQUINT64 decodedFoo64;
    decodedFoo64 = encoded;

    if (verbose) {
        qDebug() << "decodedFoo64=" << decodedFoo64.data;
        qDebug() << "decodedFoo64==foo64" << (decodedFoo64==foo64) << " { expected true } ";
    }
    testsTaken++;
    bool result12 = (decodedFoo64.data == 259);
    bool expected12 = true;
    if (result12 == expected12) {
        testsPassed++;
    } else {
        testsFailed++;
        qDebug() << "FAILED - Test 12: decodedFoo64.data == 259";
    }

    testsTaken++;
    bool result13 = (decodedFoo64==foo64);
    bool expected13 = true;
    if (result13 == expected13) {
        testsPassed++;
    } else {
        testsFailed++;
        qDebug() << "FAILED - Test 13: decodedFoo64==foo64";
    }

    if (verbose) {
        qDebug() << "ByteCountCodedQUINT64 bar64(1000000)";
    }
    ByteCountCodedQUINT64 bar64(1000000);
    encoded = bar64.encode();
    if (verbose) {
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
    }

    ByteCountCodedQUINT64 decodedBar64;
    decodedBar64.decode(encoded);
    if (verbose) {
        qDebug() << "decodedBar64=" << decodedBar64.data;
        qDebug() << "decodedBar64==bar64" << (decodedBar64==bar64) << " { expected true } ";
    }
    testsTaken++;
    bool result14 = (decodedBar64.data == 1000000);
    bool expected14 = true;
    if (result14 == expected14) {
        testsPassed++;
    } else {
        testsFailed++;
        qDebug() << "FAILED - Test 14: decodedBar64.data == 1000000";
    }

    testsTaken++;
    bool result15 = (decodedBar64==bar64);
    bool expected15 = true;
    if (result15 == expected15) {
        testsPassed++;
    } else {
        testsFailed++;
        qDebug() << "FAILED - Test 15: decodedBar64==bar64";
    }

    if (verbose) {
        qDebug() << "ByteCountCodedQUINT64 spam64(4294967295/2)";
    }
    ByteCountCodedQUINT64 spam64(4294967295/2);
    encoded = spam64.encode();
    if (verbose) {
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
    }

    ByteCountCodedQUINT64 decodedSpam64;
    decodedSpam64.decode(encoded);
    if (verbose) {
        qDebug() << "decodedSpam64=" << decodedSpam64.data;
        qDebug() << "decodedSpam64==spam64" << (decodedSpam64==spam64) << " { expected true } ";
    }
    testsTaken++;
    bool result16 = (decodedSpam64.data == 4294967295/2);
    bool expected16 = true;
    if (result16 == expected16) {
        testsPassed++;
    } else {
        testsFailed++;
        qDebug() << "FAILED - Test 16: decodedSpam64.data == 4294967295/2";
    }

    testsTaken++;
    bool result17 = (decodedSpam64==spam64);
    bool expected17 = true;
    if (result17 == expected17) {
        testsPassed++;
    } else {
        testsFailed++;
        qDebug() << "FAILED - Test 17: decodedSpam64==spam64";
    }

    if (verbose) {
        qDebug() << "testing encoded << spam64";
    }
    encoded.clear();
    encoded << spam64;
    if (verbose) {
        outputBufferBits((const unsigned char*)encoded.constData(), encoded.size());
    }

    if (verbose) {
        qDebug() << "testing encoded >> decodedSpam64";
    }
    encoded >> decodedSpam64;

    if (verbose) {
        qDebug() << "decodedSpam64=" << decodedSpam64.data;
    }
    testsTaken++;
    bool result18 = (decodedSpam64==spam64);
    bool expected18 = true;
    if (result18 == expected18) {
        testsPassed++;
    } else {
        testsFailed++;
        qDebug() << "FAILED - Test 18: decodedSpam64==spam64";
    }

    //ByteCountCodedINT shouldFail(-100);
    
    if (verbose) {
        qDebug() << "NOW...";
    }
    quint64 now = usecTimestampNow();
    ByteCountCodedQUINT64 nowCoded = now;
    QByteArray nowEncoded = nowCoded;

    if (verbose) {
        outputBufferBits((const unsigned char*)nowEncoded.constData(), nowEncoded.size());
    }
    ByteCountCodedQUINT64 decodedNow = nowEncoded;

    testsTaken++;
    bool result19 = (decodedNow.data==now);
    bool expected19 = true;
    if (result19 == expected19) {
        testsPassed++;
    } else {
        testsFailed++;
        qDebug() << "FAILED - Test 19: now test...";
    }
    
    if (verbose) {
        qDebug() << "******************************************************************************************";
    }
    qDebug() << "   tests passed:" << testsPassed << "out of" << testsTaken;
    if (verbose) {
        qDebug() << "******************************************************************************************";
    }
}

void OctreeTests::modelItemTests(bool verbose) {
    
#if 0 // TODO - repair/replace these

    //verbose = true;
    EntityTreeElementExtraEncodeData modelTreeElementExtraEncodeData;
    int testsTaken = 0;
    int testsPassed = 0;
    int testsFailed = 0;

    if (verbose) {
        qDebug() << "******************************************************************************************";
    }
    
    qDebug() << "OctreeTests::modelItemTests()";

    EncodeBitstreamParams params;
    OctreePacketData packetData;
    EntityItem entityItem;
    
    entityItem.setID(1042);
    entityItem.setModelURL("http://foo.com/foo.fbx");

    bool appendResult = entityItem.appendEntityData(&packetData, params, &modelTreeElementExtraEncodeData);
    int bytesWritten = packetData.getUncompressedSize();
    if (verbose) {
        qDebug() << "Test 1: bytesRead == bytesWritten ...";
        qDebug() << "appendResult=" << appendResult;
        qDebug() << "bytesWritten=" << bytesWritten;
    }

    {
        ReadBitstreamToTreeParams args;
        EntityItem modelItemFromBuffer;
        const unsigned char* data = packetData.getUncompressedData();
        int bytesLeftToRead = packetData.getUncompressedSize();
    
        int bytesRead =  modelItemFromBuffer.readEntityDataFromBuffer(data, bytesLeftToRead, args);
        if (verbose) {
            qDebug() << "bytesRead=" << bytesRead;
            qDebug() << "modelItemFromBuffer.getID()=" << modelItemFromBuffer.getID();
            qDebug() << "modelItemFromBuffer.getModelURL()=" << modelItemFromBuffer.getModelURL();
        }

        testsTaken++;
        bool result1 = (bytesRead == bytesWritten);
        bool expected1 = true;
        if (result1 == expected1) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 1: bytesRead == bytesWritten...";
        }

        if (verbose) {
            qDebug() << "Test 2: modelItemFromBuffer.getModelURL() == 'http://foo.com/foo.fbx'";
        }
    
        testsTaken++;
        bool result2 = (modelItemFromBuffer.getModelURL() == "http://foo.com/foo.fbx");
        bool expected2 = true;
        if (result2 == expected2) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 2: modelItemFromBuffer.getModelURL() == 'http://foo.com/foo.fbx' ...";
        }
    }
    
    // TEST 3:
    // Reset the packet, fill it with data so that EntityItem header won't fit, and verify that we don't let it fit
    {
        packetData.reset();
        int remainingSpace = 10;
        int almostFullOfData = MAX_OCTREE_UNCOMRESSED_PACKET_SIZE - remainingSpace;
        QByteArray garbageData(almostFullOfData, 0);
        packetData.appendValue(garbageData);

        appendResult = entityItem.appendEntityData(&packetData, params, &modelTreeElementExtraEncodeData);
        bytesWritten = packetData.getUncompressedSize() - almostFullOfData;
        if (verbose) {
            qDebug() << "Test 3: attempt to appendEntityData in nearly full packetData ...";
            qDebug() << "appendResult=" << appendResult;
            qDebug() << "bytesWritten=" << bytesWritten;
        }
        testsTaken++;
        bool result3 = (appendResult == false && bytesWritten == 0);
        bool expected3 = true;
        if (result3 == expected3) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 3: attempt to appendEntityData in nearly full packetData ...";
        }
    }
    
    // TEST 4:
    // Reset the packet, fill it with data so that some of EntityItem won't fit, and verify that we write what can fit
    {
        packetData.reset();
        int remainingSpace = 50;
        int almostFullOfData = MAX_OCTREE_UNCOMRESSED_PACKET_SIZE - remainingSpace;
        QByteArray garbageData(almostFullOfData, 0);
        packetData.appendValue(garbageData);

        appendResult = entityItem.appendEntityData(&packetData, params, &modelTreeElementExtraEncodeData);
        bytesWritten = packetData.getUncompressedSize() - almostFullOfData;
        if (verbose) {
            qDebug() << "Test 4: attempt to appendEntityData in nearly full packetData which some should fit ...";
            qDebug() << "appendResult=" << appendResult;
            qDebug() << "bytesWritten=" << bytesWritten;
        }
        testsTaken++;
        bool result4 = (appendResult == true); // && bytesWritten == 0);
        bool expected4 = true;
        if (result4 == expected4) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 4: attempt to appendEntityData in nearly full packetData which some should fit ...";
        }

        ReadBitstreamToTreeParams args;
        EntityItem modelItemFromBuffer;
        const unsigned char* data = packetData.getUncompressedData() + almostFullOfData;
        int bytesLeftToRead = packetData.getUncompressedSize() - almostFullOfData;
    
        int bytesRead =  modelItemFromBuffer.readEntityDataFromBuffer(data, bytesLeftToRead, args);
        if (verbose) {
            qDebug() << "Test 5: partial EntityItem written ... bytesRead == bytesWritten...";
            qDebug() << "bytesRead=" << bytesRead;
            qDebug() << "modelItemFromBuffer.getID()=" << modelItemFromBuffer.getID();
            qDebug() << "modelItemFromBuffer.getModelURL()=" << modelItemFromBuffer.getModelURL();
        }

        testsTaken++;
        bool result5 = (bytesRead == bytesWritten);
        bool expected5 = true;
        if (result5 == expected5) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 5: partial EntityItem written ... bytesRead == bytesWritten...";
        }

        if (verbose) {
            qDebug() << "Test 6: partial EntityItem written ... getModelURL() NOT SET ...";
        }
    
        testsTaken++;
        bool result6 = (modelItemFromBuffer.getModelURL() == "");
        bool expected6 = true;
        if (result6 == expected6) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test 6: partial EntityItem written ... getModelURL() NOT SET ...";
        }
    }
    
    if (verbose) {
        qDebug() << "******************************************************************************************";
    }
    qDebug() << "   tests passed:" << testsPassed << "out of" << testsTaken;
    if (verbose) {
        qDebug() << "******************************************************************************************";
    }
    
#endif 
}


void OctreeTests::runAllTests(bool verbose) {
    propertyFlagsTests(verbose);
    byteCountCodingTests(verbose);
    modelItemTests(verbose);
}

