//
//  EntityTests.h
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

#include "ModelTests.h" // needs to be EntityTests.h soon
//#include "EntityTests.h"

#include <QDebug>

#include <EntityItem.h>
#include <EntityTree.h>
#include <EntityTreeElement.h>
#include <Octree.h>
#include <OctreeConstants.h>
#include <PropertyFlags.h>
#include <SharedUtil.h>

QTEST_MAIN(EntityTests)

/*
void EntityTests::entityTreeTests(bool verbose) {

    bool extraVerbose = false;
    int testsTaken = 0;
    int testsPassed = 0;
    int testsFailed = 0;

    if (verbose) {
        qDebug() << "******************************************************************************************";
    }
    
    qDebug() << "EntityTests::entityTreeTests()";

    // Tree, id, and entity properties used in many tests below...
    EntityTree tree;
    QUuid id = QUuid::createUuid();
    EntityItemID entityID(id);
    EntityItemProperties properties;
    float oneMeter = 1.0f;
    float halfOfDomain = TREE_SCALE * 0.5f;
    glm::vec3 positionNearOrigin(oneMeter, oneMeter, oneMeter); // when using properties, these are in meter not tree units
    glm::vec3 positionAtCenter(halfOfDomain, halfOfDomain, halfOfDomain);

    {
        testsTaken++;
        QString testName = "add entity to tree and search";
        if (verbose) {
            qDebug() << "Test" << testsTaken <<":" << qPrintable(testName);
        }
        
        properties.setPosition(positionAtCenter);
        // TODO: Fix these unit tests.
        //properties.setModelURL("http://s3.amazonaws.com/hifi-public/ozan/theater.fbx");

        tree.addEntity(entityID, properties);
        
        float targetRadius = oneMeter * 2.0f;
        EntityItemPointer foundEntityByRadius = tree.findClosestEntity(positionAtCenter, targetRadius);
        EntityItemPointer foundEntityByID = tree.findEntityByEntityItemID(entityID);
        EntityTreeElement* containingElement = tree.getContainingElement(entityID);
        const AACube& elementCube = containingElement ? containingElement->getAACube() : AACube();
        
        if (verbose) {
            qDebug() << "foundEntityByRadius=" << foundEntityByRadius.get();
            qDebug() << "foundEntityByID=" << foundEntityByID.get();
            qDebug() << "containingElement=" << containingElement;
            qDebug() << "containingElement.box=" 
                << elementCube.getCorner().x << "," 
                << elementCube.getCorner().y << ","
                << elementCube.getCorner().z << ":" 
                << elementCube.getScale();
            qDebug() << "elementCube.getScale()=" << elementCube.getScale();
            //containingElement->printDebugDetails("containingElement");
        }

        bool passed = foundEntityByRadius && foundEntityByID && (foundEntityByRadius == foundEntityByID);
        if (passed) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test" << testsTaken <<":" << qPrintable(testName);
        }
    }

    {
        testsTaken++;
        QString testName = "change position of entity in tree";
        if (verbose) {
            qDebug() << "Test" << testsTaken <<":" << qPrintable(testName);
        }
        
        glm::vec3 newPosition = positionNearOrigin;

        properties.setPosition(newPosition);

        tree.updateEntity(entityID, properties);
        
        float targetRadius = oneMeter * 2.0f;
        EntityItemPointer foundEntityByRadius = tree.findClosestEntity(positionNearOrigin, targetRadius);
        EntityItemPointer foundEntityByID = tree.findEntityByEntityItemID(entityID);
        EntityTreeElement* containingElement = tree.getContainingElement(entityID);
        const AACube& elementCube = containingElement ? containingElement->getAACube() : AACube();
        
        if (verbose) {
            qDebug() << "foundEntityByRadius=" << foundEntityByRadius.get();
            qDebug() << "foundEntityByID=" << foundEntityByID.get();
            qDebug() << "containingElement=" << containingElement;
            qDebug() << "containingElement.box=" 
                << elementCube.getCorner().x << "," 
                << elementCube.getCorner().y << ","
                << elementCube.getCorner().z << ":" 
                << elementCube.getScale();
            //containingElement->printDebugDetails("containingElement");
        }

        bool passed = foundEntityByRadius && foundEntityByID && (foundEntityByRadius == foundEntityByID);
        if (passed) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test" << testsTaken <<":" << qPrintable(testName);
        }
    }

    {
        testsTaken++;
        QString testName = "change position of entity in tree back to center";
        if (verbose) {
            qDebug() << "Test" << testsTaken <<":" << qPrintable(testName);
        }
        
        glm::vec3 newPosition = positionAtCenter;

        properties.setPosition(newPosition);

        tree.updateEntity(entityID, properties);
        
        float targetRadius = oneMeter * 2.0f;
        EntityItemPointer foundEntityByRadius = tree.findClosestEntity(positionAtCenter, targetRadius);
        EntityItemPointer foundEntityByID = tree.findEntityByEntityItemID(entityID);
        EntityTreeElement* containingElement = tree.getContainingElement(entityID);
        const AACube& elementCube = containingElement ? containingElement->getAACube() : AACube();
        
        if (verbose) {
            qDebug() << "foundEntityByRadius=" << foundEntityByRadius.get();
            qDebug() << "foundEntityByID=" << foundEntityByID.get();
            qDebug() << "containingElement=" << containingElement;
            qDebug() << "containingElement.box=" 
                << elementCube.getCorner().x << "," 
                << elementCube.getCorner().y << ","
                << elementCube.getCorner().z << ":" 
                << elementCube.getScale();
            //containingElement->printDebugDetails("containingElement");
        }

        bool passed = foundEntityByRadius && foundEntityByID && (foundEntityByRadius == foundEntityByID);
        if (passed) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test" << testsTaken <<":" << qPrintable(testName);
        }
    }

    {
        testsTaken++;
        const int TEST_ITERATIONS = 1000;
        QString testName = "Performance - findClosestEntity() "+ QString::number(TEST_ITERATIONS) + " times";
        if (verbose) {
            qDebug() << "Test" << testsTaken <<":" << qPrintable(testName);
        }

        float targetRadius = oneMeter * 2.0f;
        quint64 start = usecTimestampNow();
        EntityItemPointer foundEntityByRadius = NULL;
        for (int i = 0; i < TEST_ITERATIONS; i++) {        
            foundEntityByRadius = tree.findClosestEntity(positionAtCenter, targetRadius);
        }
        quint64 end = usecTimestampNow();
        
        if (verbose) {
             qDebug() << "foundEntityByRadius=" << foundEntityByRadius.get();
        }

        bool passed = true; // foundEntityByRadius;
        if (passed) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test" << testsTaken <<":" << qPrintable(testName);
        }
        float USECS_PER_MSECS = 1000.0f;
        float elapsedInMSecs = (float)(end - start) / USECS_PER_MSECS;
        qDebug() << "TIME - Test" << testsTaken <<":" << qPrintable(testName) << "elapsed=" << elapsedInMSecs << "msecs";
    }

    {
        testsTaken++;
        const int TEST_ITERATIONS = 1000;
        QString testName = "Performance - findEntityByID() "+ QString::number(TEST_ITERATIONS) + " times";
        if (verbose) {
            qDebug() << "Test" << testsTaken <<":" << qPrintable(testName);
        }

        quint64 start = usecTimestampNow();
        EntityItemPointer foundEntityByID = NULL;
        for (int i = 0; i < TEST_ITERATIONS; i++) {
            // TODO: does this need to be updated??
            foundEntityByID = tree.findEntityByEntityItemID(entityID);
        }
        quint64 end = usecTimestampNow();
        
        if (verbose) {
            qDebug() << "foundEntityByID=" << foundEntityByID.get();
        }

        bool passed = foundEntityByID.get();
        if (passed) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test" << testsTaken <<":" << qPrintable(testName);
        }
        float USECS_PER_MSECS = 1000.0f;
        float elapsedInMSecs = (float)(end - start) / USECS_PER_MSECS;
        qDebug() << "TIME - Test" << testsTaken <<":" << qPrintable(testName) << "elapsed=" << elapsedInMSecs << "msecs";
    }

    {
        // seed the random number generator so that our tests are reproducible
        srand(0xFEEDBEEF);
    
        testsTaken++;
        const int TEST_ITERATIONS = 1000;
        QString testName = "Performance - add entity to tree " + QString::number(TEST_ITERATIONS) + " times";
        if (verbose) {
            qDebug() << "Test" << testsTaken <<":" << qPrintable(testName);
        }

        int iterationsPassed = 0;
        quint64 totalElapsedAdd = 0;
        quint64 totalElapsedFind = 0;
        for (int i = 0; i < TEST_ITERATIONS; i++) {        
            QUuid id = QUuid::createUuid();// make sure it doesn't collide with previous entity ids
            EntityItemID entityID(id);

            float randomX = randFloatInRange(1.0f ,(float)TREE_SCALE - 1.0f);
            float randomY = randFloatInRange(1.0f ,(float)TREE_SCALE - 1.0f);
            float randomZ = randFloatInRange(1.0f ,(float)TREE_SCALE - 1.0f);
            glm::vec3 randomPosition(randomX,randomY,randomZ);

            properties.setPosition(randomPosition);
            
            // TODO: fix these unit tests
            //properties.setModelURL("http://s3.amazonaws.com/hifi-public/ozan/theater.fbx");

            if (extraVerbose) {
                qDebug() << "iteration:" << i
                      << "ading entity at x/y/z=" << randomX << "," << randomY << "," << randomZ;
                qDebug() << "before:" << i << "getOctreeElementsCount()=" << tree.getOctreeElementsCount();
            }

            quint64 startAdd = usecTimestampNow();
            tree.addEntity(entityID, properties);
            quint64 endAdd = usecTimestampNow();
            totalElapsedAdd += (endAdd - startAdd);

            if (extraVerbose) {
                qDebug() << "after:" << i << "getOctreeElementsCount()=" << tree.getOctreeElementsCount();
            }

            quint64 startFind = usecTimestampNow();
            float targetRadius = oneMeter * 2.0f;
            EntityItemPointer foundEntityByRadius = tree.findClosestEntity(randomPosition, targetRadius);
            EntityItemPointer foundEntityByID = tree.findEntityByEntityItemID(entityID);
            quint64 endFind = usecTimestampNow();
            totalElapsedFind += (endFind - startFind);

            EntityTreeElement* containingElement = tree.getContainingElement(entityID);
            const AACube& elementCube = containingElement ? containingElement->getAACube() : AACube();
            
            bool elementIsBestFit = containingElement->bestFitEntityBounds(foundEntityByID);
            
            if (extraVerbose) {
                qDebug() << "foundEntityByRadius=" << foundEntityByRadius.get();
                qDebug() << "foundEntityByID=" << foundEntityByID.get();
                qDebug() << "containingElement=" << containingElement;
                qDebug() << "containingElement.box=" 
                    << elementCube.getCorner().x << "," 
                    << elementCube.getCorner().y << ","
                    << elementCube.getCorner().z << ":" 
                    << elementCube.getScale();
                qDebug() << "elementCube.getScale()=" << elementCube.getScale();
                //containingElement->printDebugDetails("containingElement");
                qDebug() << "elementIsBestFit=" << elementIsBestFit;
            }
            
            // Every 1000th test, show the size of the tree...
            if (extraVerbose && (i % 1000 == 0)) {
                qDebug() << "after test:" << i << "getOctreeElementsCount()=" << tree.getOctreeElementsCount();
            }

            bool passed = foundEntityByRadius && foundEntityByID && (foundEntityByRadius == foundEntityByID) && elementIsBestFit;
            if (passed) {
              iterationsPassed++;
            } else {
                if (extraVerbose) {
                    qDebug() << "FAILED - Test" << testsTaken <<":" << qPrintable(testName) << "iteration:" << i
                                //<< "foundEntityByRadius=" << foundEntityByRadius
                                << "foundEntityByID=" << foundEntityByID.get()
                                << "x/y/z=" << randomX << "," << randomY << "," << randomZ
                                << "elementIsBestFit=" << elementIsBestFit;
                }
            }
        }

        if (extraVerbose) {
            qDebug() << "getOctreeElementsCount()=" << tree.getOctreeElementsCount();
        }
        
        bool passed = iterationsPassed == TEST_ITERATIONS;
        if (passed) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test" << testsTaken <<":" << qPrintable(testName);
        }
        float USECS_PER_MSECS = 1000.0f;
        float elapsedInMSecsAdd = (float)(totalElapsedAdd) / USECS_PER_MSECS;
        float elapsedInMSecsFind = (float)(totalElapsedFind) / USECS_PER_MSECS;
        qDebug() << "TIME - Test" << testsTaken <<":" << qPrintable(testName) 
                        << "elapsed Add=" << elapsedInMSecsAdd << "msecs"
                        << "elapsed Find=" << elapsedInMSecsFind << "msecs";
    }

    {
        testsTaken++;
        const int TEST_ITERATIONS = 1000;
        QString testName = "Performance - delete entity from tree " + QString::number(TEST_ITERATIONS) + " times";
        if (verbose) {
            qDebug() << "Test" << testsTaken <<":" << qPrintable(testName);
        }

        int iterationsPassed = 0;
        quint64 totalElapsedDelete = 0;
        quint64 totalElapsedFind = 0;
        for (int i = 0; i < TEST_ITERATIONS; i++) {        
            QUuid id = QUuid::createUuid();// make sure it doesn't collide with previous entity ids
            EntityItemID entityID(id);

            if (extraVerbose) {
                qDebug() << "before:" << i << "getOctreeElementsCount()=" << tree.getOctreeElementsCount();
            }

            quint64 startDelete = usecTimestampNow();
            bool force = true;
            tree.deleteEntity(entityID, force);
            quint64 endDelete = usecTimestampNow();
            totalElapsedDelete += (endDelete - startDelete);

            if (extraVerbose) {
                qDebug() << "after:" << i << "getOctreeElementsCount()=" << tree.getOctreeElementsCount();
            }

            quint64 startFind = usecTimestampNow();
            EntityItemPointer foundEntityByID = tree.findEntityByEntityItemID(entityID);
            quint64 endFind = usecTimestampNow();
            totalElapsedFind += (endFind - startFind);

            EntityTreeElement* containingElement = tree.getContainingElement(entityID);
            
            if (extraVerbose) {
                 qDebug() << "foundEntityByID=" << foundEntityByID.get();
            }
            
            // Every 1000th test, show the size of the tree...
            if (extraVerbose && (i % 1000 == 0)) {
                qDebug() << "after test:" << i << "getOctreeElementsCount()=" << tree.getOctreeElementsCount();
            }

            bool passed = foundEntityByID == NULL && containingElement == NULL;
            if (passed) {
              iterationsPassed++;
            } else {
                if (extraVerbose) {
                    qDebug() << "FAILED - Test" << testsTaken <<":" << qPrintable(testName) << "iteration:" << i
                          << "containingElement=" << containingElement;
                }
            }
        }

        if (extraVerbose) {
            qDebug() << "getOctreeElementsCount()=" << tree.getOctreeElementsCount();
        }
        
        bool passed = iterationsPassed == TEST_ITERATIONS;
        if (passed) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test" << testsTaken <<":" << qPrintable(testName);
        }
        float USECS_PER_MSECS = 1000.0f;
        float elapsedInMSecsDelete = (float)(totalElapsedDelete) / USECS_PER_MSECS;
        float elapsedInMSecsFind = (float)(totalElapsedFind) / USECS_PER_MSECS;
        qDebug() << "TIME - Test" << testsTaken <<":" << qPrintable(testName) 
                        << "elapsed Delete=" << elapsedInMSecsDelete << "msecs"
                        << "elapsed Find=" << elapsedInMSecsFind << "msecs";
    }


    {
        testsTaken++;
        const int TEST_ITERATIONS = 100;
        const int ENTITIES_PER_ITERATION = 10;
        QString testName = "Performance - delete " + QString::number(ENTITIES_PER_ITERATION) 
                            + " entities from tree " + QString::number(TEST_ITERATIONS) + " times";
        if (verbose) {
            qDebug() << "Test" << testsTaken <<":" << qPrintable(testName);
        }

        int iterationsPassed = 0;
        quint64 totalElapsedDelete = 0;
        quint64 totalElapsedFind = 0;
        for (int i = 0; i < TEST_ITERATIONS; i++) {        

            std::vector<EntityItemID> entitiesToDelete;
            entitiesToDelete.reserve(ENTITIES_PER_ITERATION);
            for (int j = 0; j < ENTITIES_PER_ITERATION; j++) {        
                //uint32_t id = 2 + (i * ENTITIES_PER_ITERATION) + j; // These are the entities we added above
                QUuid id = QUuid::createUuid();// make sure it doesn't collide with previous entity ids
                EntityItemID entityID(id);
                entitiesToDelete.push_back(entityID);
            }

            if (extraVerbose) {
                qDebug() << "before:" << i << "getOctreeElementsCount()=" << tree.getOctreeElementsCount();
            }

            quint64 startDelete = usecTimestampNow();
            bool force = true;
            bool ignoreWarnings = true;
            tree.deleteEntitiesByID(entitiesToDelete, force, ignoreWarnings);
            quint64 endDelete = usecTimestampNow();
            totalElapsedDelete += (endDelete - startDelete);

            if (extraVerbose) {
                qDebug() << "after:" << i << "getOctreeElementsCount()=" << tree.getOctreeElementsCount();
            }

            quint64 startFind = usecTimestampNow();
            for (int j = 0; j < ENTITIES_PER_ITERATION; j++) {        
                //uint32_t id = 2 + (i * ENTITIES_PER_ITERATION) + j; // These are the entities we added above
                QUuid id = QUuid::createUuid();// make sure it doesn't collide with previous entity ids
                EntityItemID entityID(id);
                EntityItemPointer foundEntityByID = tree.findEntityByEntityItemID(entityID);
                EntityTreeElement* containingElement = tree.getContainingElement(entityID);

                if (extraVerbose) {
                    //qDebug() << "foundEntityByID=" << foundEntityByID;
                    qDebug() << "containingElement=" << containingElement;
                }
                bool passed = foundEntityByID == NULL && containingElement == NULL;
                if (passed) {
                  iterationsPassed++;
                } else {
                    if (extraVerbose) {
                        qDebug() << "FAILED - Test" << testsTaken <<":" << qPrintable(testName) << "iteration:" << i
                              << "containingElement=" << containingElement;
                    }
                }

            }

            quint64 endFind = usecTimestampNow();
            totalElapsedFind += (endFind - startFind);
        }

        if (extraVerbose) {
            qDebug() << "getOctreeElementsCount()=" << tree.getOctreeElementsCount();
        }
        
        bool passed = iterationsPassed == (TEST_ITERATIONS * ENTITIES_PER_ITERATION);
        if (passed) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test" << testsTaken <<":" << qPrintable(testName);
        }
        float USECS_PER_MSECS = 1000.0f;
        float elapsedInMSecsDelete = (float)(totalElapsedDelete) / USECS_PER_MSECS;
        float elapsedInMSecsFind = (float)(totalElapsedFind) / USECS_PER_MSECS;
        qDebug() << "TIME - Test" << testsTaken <<":" << qPrintable(testName) 
                        << "elapsed Delete=" << elapsedInMSecsDelete << "msecs"
                        << "elapsed Find=" << elapsedInMSecsFind << "msecs";
    }

    qDebug() << "   tests passed:" << testsPassed << "out of" << testsTaken;
    if (verbose) {
        qDebug() << "******************************************************************************************";
    }
}


void EntityTests::runAllTests(bool verbose) {
    entityTreeTests(verbose);
}
*/
