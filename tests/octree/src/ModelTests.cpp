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

#include <QDebug>

#include <EntityItem.h>
#include <EntityTree.h>
#include <EntityTreeElement.h>
#include <Octree.h>
#include <OctreeConstants.h>
#include <PropertyFlags.h>
#include <SharedUtil.h>

//#include "EntityTests.h"
#include "ModelTests.h" // needs to be EntityTests.h soon

void EntityTests::modelTreeTests(bool verbose) {
#ifdef HIDE_SUBCLASS_METHODS
    bool extraVerbose = false;
    int testsTaken = 0;
    int testsPassed = 0;
    int testsFailed = 0;

    if (verbose) {
        qDebug() << "******************************************************************************************";
    }
    
    qDebug() << "EntityTests::modelTreeTests()";

    // Tree, id, and model properties used in many tests below...
    EntityTree tree;
    uint32_t id = 1;
    EntityItemID modelID(id);
    modelID.isKnownID = false; // this is a temporary workaround to allow local tree models to be added with known IDs
    EntityItemProperties properties;
    float oneMeter = 1.0f;
    float halfMeter = oneMeter / 2.0f;
    float halfOfDomain = TREE_SCALE * 0.5f;
    glm::vec3 positionNearOriginInMeters(oneMeter, oneMeter, oneMeter); // when using properties, these are in meter not tree units
    glm::vec3 positionAtCenterInMeters(halfOfDomain, halfOfDomain, halfOfDomain);
    glm::vec3 positionNearOriginInTreeUnits = positionNearOriginInMeters / (float)TREE_SCALE;
    glm::vec3 positionAtCenterInTreeUnits = positionAtCenterInMeters / (float)TREE_SCALE;

    {
        testsTaken++;
        QString testName = "add model to tree and search";
        if (verbose) {
            qDebug() << "Test" << testsTaken <<":" << qPrintable(testName);
        }
        
        properties.setPosition(positionAtCenterInMeters);
        properties.setRadius(halfMeter);
//properties.setModelURL("https://s3-us-west-1.amazonaws.com/highfidelity-public/ozan/theater.fbx");

        tree.addEntity(modelID, properties);
        
        float targetRadius = oneMeter * 2.0 / (float)TREE_SCALE; // in tree units
        const EntityItem* foundEntityByRadius = tree.findClosestEntity(positionAtCenterInTreeUnits, targetRadius);
        const EntityItem* foundEntityByID = tree.findEntityByID(id);
        EntityTreeElement* containingElement = tree.getContainingElement(modelID);
        AACube elementCube = containingElement ? containingElement->getAACube() : AACube();
        
        if (verbose) {
            qDebug() << "foundEntityByRadius=" << foundEntityByRadius;
            qDebug() << "foundEntityByID=" << foundEntityByID;
            qDebug() << "containingElement=" << containingElement;
            qDebug() << "containingElement.box=" 
                << elementCube.getCorner().x * TREE_SCALE << "," 
                << elementCube.getCorner().y * TREE_SCALE << ","
                << elementCube.getCorner().z * TREE_SCALE << ":" 
                << elementCube.getScale() * TREE_SCALE;
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

    modelID.isKnownID = true; // this is a temporary workaround to allow local tree models to be added with known IDs

    {
        testsTaken++;
        QString testName = "change position of model in tree";
        if (verbose) {
            qDebug() << "Test" << testsTaken <<":" << qPrintable(testName);
        }
        
        glm::vec3 newPosition = positionNearOriginInMeters;

        properties.setPosition(newPosition);

        tree.updateEntity(modelID, properties);
        
        float targetRadius = oneMeter * 2.0 / (float)TREE_SCALE; // in tree units
        const EntityItem* foundEntityByRadius = tree.findClosestEntity(positionNearOriginInTreeUnits, targetRadius);
        const EntityItem* foundEntityByID = tree.findEntityByID(id);
        EntityTreeElement* containingElement = tree.getContainingElement(modelID);
        AACube elementCube = containingElement ? containingElement->getAACube() : AACube();
        
        if (verbose) {
            qDebug() << "foundEntityByRadius=" << foundEntityByRadius;
            qDebug() << "foundEntityByID=" << foundEntityByID;
            qDebug() << "containingElement=" << containingElement;
            qDebug() << "containingElement.box=" 
                << elementCube.getCorner().x * TREE_SCALE << "," 
                << elementCube.getCorner().y * TREE_SCALE << ","
                << elementCube.getCorner().z * TREE_SCALE << ":" 
                << elementCube.getScale() * TREE_SCALE;
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
        QString testName = "change position of model in tree back to center";
        if (verbose) {
            qDebug() << "Test" << testsTaken <<":" << qPrintable(testName);
        }
        
        glm::vec3 newPosition = positionAtCenterInMeters;

        properties.setPosition(newPosition);

        tree.updateEntity(modelID, properties);
        
        float targetRadius = oneMeter * 2.0 / (float)TREE_SCALE; // in tree units
        const EntityItem* foundEntityByRadius = tree.findClosestEntity(positionAtCenterInTreeUnits, targetRadius);
        const EntityItem* foundEntityByID = tree.findEntityByID(id);
        EntityTreeElement* containingElement = tree.getContainingElement(modelID);
        AACube elementCube = containingElement ? containingElement->getAACube() : AACube();
        
        if (verbose) {
            qDebug() << "foundEntityByRadius=" << foundEntityByRadius;
            qDebug() << "foundEntityByID=" << foundEntityByID;
            qDebug() << "containingElement=" << containingElement;
            qDebug() << "containingElement.box=" 
                << elementCube.getCorner().x * TREE_SCALE << "," 
                << elementCube.getCorner().y * TREE_SCALE << ","
                << elementCube.getCorner().z * TREE_SCALE << ":" 
                << elementCube.getScale() * TREE_SCALE;
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

        float targetRadius = oneMeter * 2.0 / (float)TREE_SCALE; // in tree units
        quint64 start = usecTimestampNow();
        const EntityItem* foundEntityByRadius = NULL;
        for (int i = 0; i < TEST_ITERATIONS; i++) {        
            foundEntityByRadius = tree.findClosestEntity(positionAtCenterInTreeUnits, targetRadius);
        }
        quint64 end = usecTimestampNow();
        
        if (verbose) {
            qDebug() << "foundEntityByRadius=" << foundEntityByRadius;
        }

        bool passed = foundEntityByRadius;
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
        const EntityItem* foundEntityByID = NULL;
        for (int i = 0; i < TEST_ITERATIONS; i++) {        
            foundEntityByID = tree.findEntityByID(id);
        }
        quint64 end = usecTimestampNow();
        
        if (verbose) {
            qDebug() << "foundEntityByID=" << foundEntityByID;
        }

        bool passed = foundEntityByID;
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
        QString testName = "Performance - add model to tree " + QString::number(TEST_ITERATIONS) + " times";
        if (verbose) {
            qDebug() << "Test" << testsTaken <<":" << qPrintable(testName);
        }

        int iterationsPassed = 0;
        quint64 totalElapsedAdd = 0;
        quint64 totalElapsedFind = 0;
        for (int i = 0; i < TEST_ITERATIONS; i++) {        
            uint32_t id = i + 2; // make sure it doesn't collide with previous model ids
            EntityItemID modelID(id);
            modelID.isKnownID = false; // this is a temporary workaround to allow local tree models to be added with known IDs

            float randomX = randFloatInRange(1.0f ,(float)TREE_SCALE - 1.0f);
            float randomY = randFloatInRange(1.0f ,(float)TREE_SCALE - 1.0f);
            float randomZ = randFloatInRange(1.0f ,(float)TREE_SCALE - 1.0f);
            glm::vec3 randomPositionInMeters(randomX,randomY,randomZ);
            glm::vec3 randomPositionInTreeUnits = randomPositionInMeters / (float)TREE_SCALE;

            properties.setPosition(randomPositionInMeters);
            properties.setRadius(halfMeter);
//properties.setModelURL("https://s3-us-west-1.amazonaws.com/highfidelity-public/ozan/theater.fbx");

            if (extraVerbose) {
                qDebug() << "iteration:" << i
                      << "ading model at x/y/z=" << randomX << "," << randomY << "," << randomZ;
                qDebug() << "before:" << i << "getOctreeElementsCount()=" << tree.getOctreeElementsCount();
            }

            quint64 startAdd = usecTimestampNow();
            tree.addEntity(modelID, properties);
            quint64 endAdd = usecTimestampNow();
            totalElapsedAdd += (endAdd - startAdd);

            if (extraVerbose) {
                qDebug() << "after:" << i << "getOctreeElementsCount()=" << tree.getOctreeElementsCount();
            }

            quint64 startFind = usecTimestampNow();
            float targetRadius = oneMeter * 2.0 / (float)TREE_SCALE; // in tree units
            const EntityItem* foundEntityByRadius = tree.findClosestEntity(randomPositionInTreeUnits, targetRadius);
            const EntityItem* foundEntityByID = tree.findEntityByID(id);
            quint64 endFind = usecTimestampNow();
            totalElapsedFind += (endFind - startFind);

            EntityTreeElement* containingElement = tree.getContainingElement(modelID);
            AACube elementCube = containingElement ? containingElement->getAACube() : AACube();
            
            bool elementIsBestFit = containingElement->bestFitEntityBounds(foundEntityByID);
            
            if (extraVerbose) {
                qDebug() << "foundEntityByRadius=" << foundEntityByRadius;
                qDebug() << "foundEntityByID=" << foundEntityByID;
                qDebug() << "containingElement=" << containingElement;
                qDebug() << "containingElement.box=" 
                    << elementCube.getCorner().x * TREE_SCALE << "," 
                    << elementCube.getCorner().y * TREE_SCALE << ","
                    << elementCube.getCorner().z * TREE_SCALE << ":" 
                    << elementCube.getScale() * TREE_SCALE;
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
                          << "foundEntityByRadius=" << foundEntityByRadius << "foundEntityByID=" << foundEntityByID
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
        QString testName = "Performance - delete model from tree " + QString::number(TEST_ITERATIONS) + " times";
        if (verbose) {
            qDebug() << "Test" << testsTaken <<":" << qPrintable(testName);
        }

        int iterationsPassed = 0;
        quint64 totalElapsedDelete = 0;
        quint64 totalElapsedFind = 0;
        for (int i = 0; i < TEST_ITERATIONS; i++) {        
            uint32_t id = i + 2; // These are the models we added above
            EntityItemID modelID(id);
            modelID.isKnownID = true; // this is a temporary workaround to allow local tree models to be added with known IDs

            if (extraVerbose) {
                qDebug() << "before:" << i << "getOctreeElementsCount()=" << tree.getOctreeElementsCount();
            }

            quint64 startDelete = usecTimestampNow();
            tree.deleteEntity(modelID);
            quint64 endDelete = usecTimestampNow();
            totalElapsedDelete += (endDelete - startDelete);

            if (extraVerbose) {
                qDebug() << "after:" << i << "getOctreeElementsCount()=" << tree.getOctreeElementsCount();
            }

            quint64 startFind = usecTimestampNow();
            const EntityItem* foundEntityByID = tree.findEntityByID(id);
            quint64 endFind = usecTimestampNow();
            totalElapsedFind += (endFind - startFind);

            EntityTreeElement* containingElement = tree.getContainingElement(modelID);
            
            if (extraVerbose) {
                qDebug() << "foundEntityByID=" << foundEntityByID;
                qDebug() << "containingElement=" << containingElement;
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
                          << "foundEntityByID=" << foundEntityByID
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
        const int MODELS_PER_ITERATION = 10;
        QString testName = "Performance - delete " + QString::number(MODELS_PER_ITERATION) 
                            + " models from tree " + QString::number(TEST_ITERATIONS) + " times";
        if (verbose) {
            qDebug() << "Test" << testsTaken <<":" << qPrintable(testName);
        }

        int iterationsPassed = 0;
        quint64 totalElapsedDelete = 0;
        quint64 totalElapsedFind = 0;
        for (int i = 0; i < TEST_ITERATIONS; i++) {        

            QSet<EntityItemID> modelsToDelete;
            for (int j = 0; j < MODELS_PER_ITERATION; j++) {        
                uint32_t id = 2 + (i * MODELS_PER_ITERATION) + j; // These are the models we added above
                EntityItemID modelID(id);
                modelsToDelete << modelID;
            }

            if (extraVerbose) {
                qDebug() << "before:" << i << "getOctreeElementsCount()=" << tree.getOctreeElementsCount();
            }

            quint64 startDelete = usecTimestampNow();
            tree.deleteEntitys(modelsToDelete);
            quint64 endDelete = usecTimestampNow();
            totalElapsedDelete += (endDelete - startDelete);

            if (extraVerbose) {
                qDebug() << "after:" << i << "getOctreeElementsCount()=" << tree.getOctreeElementsCount();
            }

            quint64 startFind = usecTimestampNow();
            for (int j = 0; j < MODELS_PER_ITERATION; j++) {        
                uint32_t id = 2 + (i * MODELS_PER_ITERATION) + j; // These are the models we added above
                EntityItemID modelID(id);
                const EntityItem* foundEntityByID = tree.findEntityByID(id);
                EntityTreeElement* containingElement = tree.getContainingElement(modelID);

                if (extraVerbose) {
                    qDebug() << "foundEntityByID=" << foundEntityByID;
                    qDebug() << "containingElement=" << containingElement;
                }
                bool passed = foundEntityByID == NULL && containingElement == NULL;
                if (passed) {
                  iterationsPassed++;
                } else {
                    if (extraVerbose) {
                        qDebug() << "FAILED - Test" << testsTaken <<":" << qPrintable(testName) << "iteration:" << i
                              << "foundEntityByID=" << foundEntityByID
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
        
        bool passed = iterationsPassed == (TEST_ITERATIONS * MODELS_PER_ITERATION);
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
#endif
}


void EntityTests::runAllTests(bool verbose) {
    modelTreeTests(verbose);
}

