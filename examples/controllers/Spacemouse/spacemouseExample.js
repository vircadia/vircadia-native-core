//
// spaceMouseDebug.js
//  examples
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//



var firstmove = 1;
var position = {
                x: 0,
                y: 0,
                z: 0
            };
var rotation = {
                x: 0,
                y: 0,
                z: 0
            };

function toggleFirstMove() {
    if(firstmove){
                print("____________________________________");
                firstmove = 0;
     }
}

function spacemouseCheck() {
    return Controller.Hardware.Spacemouse !== undefined;
}

function update(deltaTime) {
    if(spacemouseCheck){        
        if(Controller.getValue(Controller.Standard.LY) != 0){  
            toggleFirstMove();
            print("- Controller TY: " + Controller.getValue(Controller.Standard.LY));
        }            
        
        if(Controller.getValue(Controller.Standard.LX) != 0){
            toggleFirstMove();
            print("- Controller RZ: " + Controller.getValue(Controller.Standard.LX));
        }
        
        if(Controller.getValue(Controller.Standard.LB) != 0){
            toggleFirstMove();
            print("- Controller LEFTB: " + Controller.getValue(Controller.Standard.LB));
        }
        
        if(Controller.getValue(Controller.Standard.RY) != 0){
            toggleFirstMove();
            print("- Controller TZ: " + Controller.getValue(Controller.Standard.RY));
        }
        
        if(Controller.getValue(Controller.Standard.RX) != 0){
            toggleFirstMove();
            print("- Controller TX: " + Controller.getValue(Controller.Standard.RX));
        }
        
        if(Controller.getValue(Controller.Standard.RB) != 0){
            toggleFirstMove();
            print("- Controller RIGHTB: " + Controller.getValue(Controller.Standard.RB));
        }    
        
        firstmove = 1;
    }
}

Script.update.connect(update);