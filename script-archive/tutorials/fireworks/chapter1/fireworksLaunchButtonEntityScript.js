//
//  fireworksLaunchButtonEntityScript.js
//
//  Created by Eric Levin on 3/7/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  This is the chapter 1 entity script of the fireworks tutorial (https://docs.vircadia.dev/docs/fireworks-scripting-tutorial)
//
//  Distributed under the Apache License, Version 2.0.

  (function() {
    Script.include("../../libraries/utils.js");
    var _this;
    Fireworks = function() {
      _this = this;
    };

    Fireworks.prototype = {

      preload: function(entityID) {
        _this.entityID = entityID;

      }

    };

    // entity scripts always need to return a newly constructed object of our type
    return new Fireworks();
  });
