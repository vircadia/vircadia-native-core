//
//  goto.js
//  scripts/system/
//
//  Created by Howard Stearns on 2 Jun 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var toolBar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");


var button = toolBar.addButton({
    objectName: "mute",
    imageURL: Script.resolvePath("assets/images/tools/microphone.svg"),
    visible: true,
    alpha: 0.9,
});
    
button.clicked.connect(function(){
    var menuItem = "Mute Microphone"; 
    Menu.setIsOptionChecked(menuItem, !Menu.isOptionChecked(menuItem));
});

Script.scriptEnding.connect(function () {
    button.clicked.disconnect();    
});
