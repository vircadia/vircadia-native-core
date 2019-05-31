//
//  Performance Settings.qml
//
//  Created by Sam Gateau on 5/28/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../../lib/prop" as Prop

Column {
    anchors.left: parent.left 
    anchors.right: parent.right 

    Prop.PropString {
        label: "Platform Tier"
        //object: Performance
        valueVarSetter: function (v) {}
        valueVarGetter: function () {
            return PlatformInfo.getPlatformTierNames()[PlatformInfo.getTierProfiled()]; } 
    }

    Prop.PropEnum {
        label: "Performance Preset"
        //object: Performance
        valueVarSetter: Performance.setPerformancePreset 
        valueVarGetter: Performance.getPerformancePreset 
        enums: Performance.getPerformancePresetNames()
    }

    Prop.PropEnum {
        label: "Refresh Rate Profile"
        //object: Performance
        valueVarSetter: Performance.setRefreshRateProfile 
        valueVarGetter: Performance.getRefreshRateProfile 
        enums: Performance.getRefreshRateProfileNames()
    }
}
