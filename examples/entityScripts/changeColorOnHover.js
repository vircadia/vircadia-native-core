//
//  changeColorOnHover.js
//  examples/entityScripts
//
//  Created by Brad Hefta-Gaub on 11/1/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example of an entity script which when assigned to a non-model entity like a box or sphere, will
//  change the color of the entity when you hover over it. This script uses the JavaScript prototype/class functionality
//  to construct an object that has methods for hoverEnterEntity and hoverLeaveEntity;
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    Script.include("changeColorOnHoverClass.js");
    return new ChangeColorOnHover();
})
