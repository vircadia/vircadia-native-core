//
//  platform.qml
//
//  Created by Sam Gateau on 5/25/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import controlsUit 1.0 as HifiControls

import "../../lib/prop" as Prop

Column {
    width: parent.width

    Prop.PropGroup {
        id: computer
        label: "Computer"
        isUnfold: true

        Component.onCompleted: {    
            computer.populateFromObjectProps(JSON.parse(PlatformInfo.getComputer()))
        }
    }
    Prop.PropGroup {
        id: cpu
        label: "CPU"
        isUnfold: true

        Component.onCompleted: {    
            cpu.populateFromObjectProps(JSON.parse(PlatformInfo.getCPU(0)))
        }
    }
    Prop.PropGroup {
        id: memory
        label: "Memory"
        isUnfold: true

        Component.onCompleted: {    
            memory.populateFromObjectProps(JSON.parse(PlatformInfo.getMemory()))
        }
    }
    Prop.PropGroup {
        id: gpu
        label: "GPU"
        isUnfold: true

        Component.onCompleted: {    
            gpu.populateFromObjectProps(JSON.parse(PlatformInfo.getGPU(0)))
        }
    }
    Prop.PropGroup {
        id: display
        label: "Display"
        isUnfold: true

        Component.onCompleted: {    
            display.populateFromObjectProps(JSON.parse(PlatformInfo.getDisplay(0)))
        }
    }
}

