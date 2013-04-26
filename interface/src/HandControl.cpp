//
//  HandControl.cpp
//  interface
//
//  Created by Jeffrey Ventrella
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "HandControl.h"

// this class takes mouse movements normalized within the screen 
// dimensions and uses those to determine avatar hand movements, as well
// as states for ramping up and ramping down the amplitude of such movements. 
// 
// This class might expand to accommodate 3D input devices 
//

HandControl::HandControl() {
   
   _enabled      = false;
   _width        = 0;
   _height       = 0;
   _startX       = 0;
   _startY       = 0;
   _x            = 0; 
   _y            = 0;
   _lastX        = 0; 
   _lastY        = 0;
   _velocityX    = 0;
   _velocityY    = 0;
   _rampUpRate   = 0.05;
   _rampDownRate = 0.02;
   _envelope     = 0.0f;
}

void HandControl::setScreenDimensions( int width, int height ) {
   _width  = width;
   _height = height;
   _startX = _width  / 2;
   _startY = _height / 2;
}

void HandControl::update( int x, int y ) {
    _lastX = _x;
    _lastY = _y;
    _x = x;
    _y = y;
    _velocityX = _x - _lastX;
    _velocityY = _y - _lastY;

    // if the mouse is moving, ramp up the envelope to increase amplitude of hand movement...
    if (( _velocityX != 0 )
    ||  ( _velocityY != 0 )) {
        _enabled = true;
        if ( _envelope < 1.0 ) {
            _envelope += _rampUpRate;
            if ( _envelope >= 1.0 ) { 
                _envelope = 1.0; 
            }
        }
    }

    // if not enabled ramp down the envelope to decrease amplitude of hand movement...
   if ( ! _enabled ) {
        if ( _envelope > 0.0 ) {
            _envelope -= _rampDownRate;
            if ( _envelope <= 0.0 ) { 
                _startX = _width  / 2;
                _startY = _height / 2;
                _envelope = 0.0; 
            }
        }
    }
    
    _leftRight = 0.0;
    _downUp    = 0.0;
    _backFront = 0.0;			
    
    // if envelope is greater than zero, apply mouse movement to values to be output 
    if ( _envelope > 0.0 ) {
        _leftRight += ( ( _x - _startX ) / (float)_width  ) * _envelope;
        _downUp    += ( ( _y - _startY ) / (float)_height ) * _envelope;
    }
}

glm::vec3 HandControl::getValues() {
    return glm::vec3( _leftRight, _downUp, _backFront );
}

void HandControl::stop() {
    _enabled = false;
}

