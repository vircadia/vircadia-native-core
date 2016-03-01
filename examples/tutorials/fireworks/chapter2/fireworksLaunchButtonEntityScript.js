// Chapter 1: fireworksLaunchButtonEntityScript.js

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
