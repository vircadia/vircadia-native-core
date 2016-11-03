//
//  faceBlendCoefficients.js
//
//  version 2.0
//
//  Created by Bob Long, 9/14/2015
//  A simple panel that can select and display the blending coefficient of the Avatar's face model.
//  
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include('utilities/tools/cookies.js')

var panel;
var coeff;
var interval;
var item = 0;
var DEVELOPER_MENU = "Developer";
var AVATAR_MENU = DEVELOPER_MENU + " > Avatar";
var SHOW_FACE_BLEND_COEFFICIENTS = "Show face blend coefficients"

function MenuConnect(menuItem) {
    if (menuItem == SHOW_FACE_BLEND_COEFFICIENTS) {
        if(Menu.isOptionChecked(SHOW_FACE_BLEND_COEFFICIENTS)) {
           panel.show();
           Overlays.editOverlay(coeff, { visible : true });
        } else {
           panel.hide();
           Overlays.editOverlay(coeff, { visible : false });
        }        
    }
}

// Add a menu item to show/hide the coefficients
function setupMenu() {   
   if (!Menu.menuExists(DEVELOPER_MENU)) {
     Menu.addMenu(DEVELOPER_MENU);
   }
   
   if (!Menu.menuExists(AVATAR_MENU)) {
     Menu.addMenu(AVATAR_MENU);
   }
   
   Menu.addMenuItem({ menuName: AVATAR_MENU, menuItemName: SHOW_FACE_BLEND_COEFFICIENTS, isCheckable: true, isChecked: true });
   Menu.menuItemEvent.connect(MenuConnect);
}

function setupPanel() {
   panel = new Panel(10, 400);   
   
   // Slider to select which coefficient to display
   panel.newSlider("Select Coefficient Index",
       0,
       100,
       function(value) { item = value.toFixed(0); },
       function() { return item; },
       function(value) { return "index = " + item; }
   );
   
   // The raw overlay used to show the actual coefficient value
   coeff = Overlays.addOverlay("text", {
       x: 10,
       y: 420,
       width: 300,
       height: 50,
       color: { red: 255, green: 255, blue: 255 },
       alpha: 1.0,
       backgroundColor: { red: 127, green: 127, blue: 127 },
       backgroundAlpha: 0.5,
       topMargin: 15,
       leftMargin: 20,
       text: "Coefficient: 0.0"
   });
    
    // Set up the interval (0.5 sec) to update the coefficient.
   interval = Script.setInterval(function() {
      Overlays.editOverlay(coeff, { text: "Coefficient: " + MyAvatar.getFaceBlendCoef(item).toFixed(4) });
   }, 500);
   
   // Mouse event setup
   Controller.mouseMoveEvent.connect(function panelMouseMoveEvent(event) { return panel.mouseMoveEvent(event); });
   Controller.mousePressEvent.connect( function panelMousePressEvent(event) { return panel.mousePressEvent(event); });
   Controller.mouseReleaseEvent.connect(function(event) { return panel.mouseReleaseEvent(event); });
}

// Clean up
function scriptEnding() {   
   panel.destroy();
   Overlays.deleteOverlay(coeff);
   Script.clearInterval(interval);
   
   Menu.removeMenuItem(AVATAR_MENU, SHOW_FACE_BLEND_COEFFICIENTS);
}

setupMenu();
setupPanel();
Script.scriptEnding.connect(scriptEnding);