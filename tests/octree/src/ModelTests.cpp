//
//  ModelTests.h
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

#include <ModelItem.h>
#include <ModelTree.h>
#include <ModelTreeElement.h>
#include <Octree.h>
#include <OctreeConstants.h>
#include <PropertyFlags.h>
#include <SharedUtil.h>

#include "ModelTests.h"


void ModelTests::modelTreeTests(bool verbose) {
    bool extraVerbose = false;
    int testsTaken = 0;
    int testsPassed = 0;
    int testsFailed = 0;

    if (verbose) {
        qDebug() << "******************************************************************************************";
    }
    
    qDebug() << "ModelTests::modelTreeTests()";

    // Tree, id, and model properties used in many tests below...
    ModelTree tree;
    uint32_t id = 1;
    ModelItemID modelID(id);
    modelID.isKnownID = false; // this is a temporary workaround to allow local tree models to be added with known IDs
    ModelItemProperties properties;
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
        properties.setModelURL("https://s3-us-west-1.amazonaws.com/highfidelity-public/ozan/theater.fbx");

        tree.addModel(modelID, properties);
        
        float targetRadius = oneMeter * 2.0 / (float)TREE_SCALE; // in tree units
        const ModelItem* foundModelByRadius = tree.findClosestModel(positionAtCenterInTreeUnits, targetRadius);
        const ModelItem* foundModelByID = tree.findModelByID(id);
        ModelTreeElement* containingElement = tree.getContainingElement(modelID);
        AACube elementCube = containingElement ? containingElement->getAACube() : AACube();
        
        if (verbose) {
            qDebug() << "foundModelByRadius=" << foundModelByRadius;
            qDebug() << "foundModelByID=" << foundModelByID;
            qDebug() << "containingElement=" << containingElement;
            qDebug() << "containingElement.box=" 
                << elementCube.getCorner().x * TREE_SCALE << "," 
                << elementCube.getCorner().y * TREE_SCALE << ","
                << elementCube.getCorner().z * TREE_SCALE << ":" 
                << elementCube.getScale() * TREE_SCALE;
            qDebug() << "elementCube.getScale()=" << elementCube.getScale();
            //containingElement->printDebugDetails("containingElement");
        }

        bool passed = foundModelByRadius && foundModelByID && (foundModelByRadius == foundModelByID);
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

        tree.updateModel(modelID, properties);
        
        float targetRadius = oneMeter * 2.0 / (float)TREE_SCALE; // in tree units
        const ModelItem* foundModelByRadius = tree.findClosestModel(positionNearOriginInTreeUnits, targetRadius);
        const ModelItem* foundModelByID = tree.findModelByID(id);
        ModelTreeElement* containingElement = tree.getContainingElement(modelID);
        AACube elementCube = containingElement ? containingElement->getAACube() : AACube();
        
        if (verbose) {
            qDebug() << "foundModelByRadius=" << foundModelByRadius;
            qDebug() << "foundModelByID=" << foundModelByID;
            qDebug() << "containingElement=" << containingElement;
            qDebug() << "containingElement.box=" 
                << elementCube.getCorner().x * TREE_SCALE << "," 
                << elementCube.getCorner().y * TREE_SCALE << ","
                << elementCube.getCorner().z * TREE_SCALE << ":" 
                << elementCube.getScale() * TREE_SCALE;
            //containingElement->printDebugDetails("containingElement");
        }

        bool passed = foundModelByRadius && foundModelByID && (foundModelByRadius == foundModelByID);
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

        tree.updateModel(modelID, properties);
        
        float targetRadius = oneMeter * 2.0 / (float)TREE_SCALE; // in tree units
        const ModelItem* foundModelByRadius = tree.findClosestModel(positionAtCenterInTreeUnits, targetRadius);
        const ModelItem* foundModelByID = tree.findModelByID(id);
        ModelTreeElement* containingElement = tree.getContainingElement(modelID);
        AACube elementCube = containingElement ? containingElement->getAACube() : AACube();
        
        if (verbose) {
            qDebug() << "foundModelByRadius=" << foundModelByRadius;
            qDebug() << "foundModelByID=" << foundModelByID;
            qDebug() << "containingElement=" << containingElement;
            qDebug() << "containingElement.box=" 
                << elementCube.getCorner().x * TREE_SCALE << "," 
                << elementCube.getCorner().y * TREE_SCALE << ","
                << elementCube.getCorner().z * TREE_SCALE << ":" 
                << elementCube.getScale() * TREE_SCALE;
            //containingElement->printDebugDetails("containingElement");
        }

        bool passed = foundModelByRadius && foundModelByID && (foundModelByRadius == foundModelByID);
        if (passed) {
            testsPassed++;
        } else {
            testsFailed++;
            qDebug() << "FAILED - Test" << testsTaken <<":" << qPrintable(testName);
        }
    }

    {
        testsTaken++;
        QString testName = "Performance - findClosestModel() 1,000,000 times";
        if (verbose) {
            qDebug() << "Test" << testsTaken <<":" << qPrintable(testName);
        }

        float targetRadius = oneMeter * 2.0 / (float)TREE_SCALE; // in tree units
        const int TEST_ITERATIONS = 1000000;
        quint64 start = usecTimestampNow();
        const ModelItem* foundModelByRadius = NULL;
        for (int i = 0; i < TEST_ITERATIONS; i++) {        
            foundModelByRadius = tree.findClosestModel(positionAtCenterInTreeUnits, targetRadius);
        }
        quint64 end = usecTimestampNow();
        
        if (verbose) {
            qDebug() << "foundModelByRadius=" << foundModelByRadius;
        }

        bool passed = foundModelByRadius;
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
        QString testName = "Performance - findModelByID() 1,000,000 times";
        if (verbose) {
            qDebug() << "Test" << testsTaken <<":" << qPrintable(testName);
        }

        const int TEST_ITERATIONS = 1000000;
        quint64 start = usecTimestampNow();
        const ModelItem* foundModelByID = NULL;
        for (int i = 0; i < TEST_ITERATIONS; i++) {        
            foundModelByID = tree.findModelByID(id);
        }
        quint64 end = usecTimestampNow();
        
        if (verbose) {
            qDebug() << "foundModelByID=" << foundModelByID;
        }

        bool passed = foundModelByID;
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
        ModelTree tree;
    
        testsTaken++;
        const int TEST_ITERATIONS = 10000;
        QString testName = "Performance - add model to tree " + QString::number(TEST_ITERATIONS) + " times";
        if (verbose) {
            qDebug() << "Test" << testsTaken <<":" << qPrintable(testName);
        }

        int iterationsPassed = 0;
        quint64 totalElapsedAdd = 0;
        quint64 totalElapsedFind = 0;
        for (int i = 0; i < TEST_ITERATIONS; i++) {        
            uint32_t id = i + 2; // make sure it doesn't collide with previous model ids
            ModelItemID modelID(id);
            modelID.isKnownID = false; // this is a temporary workaround to allow local tree models to be added with known IDs

            float randomX = randFloatInRange(1.0f ,(float)TREE_SCALE - 1.0f);
            float randomY = randFloatInRange(1.0f ,(float)TREE_SCALE - 1.0f);
            float randomZ = randFloatInRange(1.0f ,(float)TREE_SCALE - 1.0f);
            glm::vec3 randomPositionInMeters(randomX,randomY,randomZ);
            glm::vec3 randomPositionInTreeUnits = randomPositionInMeters / (float)TREE_SCALE;

            properties.setPosition(randomPositionInMeters);
            properties.setRadius(halfMeter);
            properties.setModelURL("https://s3-us-west-1.amazonaws.com/highfidelity-public/ozan/theater.fbx");

            if (extraVerbose) {
                qDebug() << "iteration:" << i
                      << "ading model at x/y/z=" << randomX << "," << randomY << "," << randomZ;
                qDebug() << "before:" << i << "getOctreeElementsCount()=" << tree.getOctreeElementsCount();
            }

            quint64 startAdd = usecTimestampNow();
            tree.addModel(modelID, properties);
            quint64 endAdd = usecTimestampNow();
            totalElapsedAdd += (endAdd - startAdd);

            if (extraVerbose) {
                qDebug() << "after:" << i << "getOctreeElementsCount()=" << tree.getOctreeElementsCount();
            }

            quint64 startFind = usecTimestampNow();
            float targetRadius = oneMeter * 2.0 / (float)TREE_SCALE; // in tree units
            const ModelItem* foundModelByRadius = tree.findClosestModel(randomPositionInTreeUnits, targetRadius);
            const ModelItem* foundModelByID = tree.findModelByID(id);
            quint64 endFind = usecTimestampNow();
            totalElapsedFind += (endFind - startFind);

            ModelTreeElement* containingElement = tree.getContainingElement(modelID);
            AACube elementCube = containingElement ? containingElement->getAACube() : AACube();
            
            bool elementIsBestFit = containingElement->bestFitModelBounds(*foundModelByID);
            
            if (extraVerbose) {
                qDebug() << "foundModelByRadius=" << foundModelByRadius;
                qDebug() << "foundModelByID=" << foundModelByID;
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
            if (verbose && (i % 1000 == 0)) {
                qDebug() << "after test:" << i << "getOctreeElementsCount()=" << tree.getOctreeElementsCount();
            }

            bool passed = foundModelByRadius && foundModelByID && (foundModelByRadius == foundModelByID) && elementIsBestFit;
            if (passed) {
              iterationsPassed++;
            } else {
              qDebug() << "FAILED - Test" << testsTaken <<":" << qPrintable(testName) << "iteration:" << i
                  << "foundModelByRadius=" << foundModelByRadius << "foundModelByID=" << foundModelByID
                  << "x/y/z=" << randomX << "," << randomY << "," << randomZ
                  << "elementIsBestFit=" << elementIsBestFit;
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

    qDebug() << "   tests passed:" << testsPassed << "out of" << testsTaken;
    if (verbose) {
        qDebug() << "******************************************************************************************";
    }
}


void ModelTests::runAllTests(bool verbose) {
    modelTreeTests(verbose);
}

