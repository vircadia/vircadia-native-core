//
//  globalLight.qml
//  examples/utilities/render
//
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4
import "configSlider"

Column {
    id: root
    spacing: 8
    property var currentZoneID
    property var zoneProperties

     Component.onCompleted: {
        Entities.getProperties
        sceneOctree.enabled = true;
        itemSelection.enabled = true;                    
        sceneOctree.showVisibleCells = false;
        sceneOctree.showEmptyCells = false;
        itemSelection.showInsideItems = false;
        itemSelection.showInsideSubcellItems = false;
        itemSelection.showPartialItems = false;
        itemSelection.showPartialSubcellItems = false;
    }
    Component.onDestruction: {
        sceneOctree.enabled = false;
        itemSelection.enabled = false;  
        Render.getConfig("FetchSceneSelection").freezeFrustum = false;
        Render.getConfig("CullSceneSelection").freezeFrustum = false;                     
    }
