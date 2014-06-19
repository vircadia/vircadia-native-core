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

    qDebug() << "   tests passed:" << testsPassed << "out of" << testsTaken;
    if (verbose) {
        qDebug() << "******************************************************************************************";
    }
}


void ModelTests::runAllTests(bool verbose) {
    modelTreeTests(verbose);
}

