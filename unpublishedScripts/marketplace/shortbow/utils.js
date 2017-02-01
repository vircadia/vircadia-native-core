if (!Function.prototype.bind) {
  Function.prototype.bind = function(oThis) {
    if (typeof this !== 'function') {
      // closest thing possible to the ECMAScript 5
      // internal IsCallable function
      throw new TypeError('Function.prototype.bind - what is trying to be bound is not callable');
    }

    var aArgs   = Array.prototype.slice.call(arguments, 1),
        fToBind = this,
        fNOP    = function() {},
        fBound  = function() {
          return fToBind.apply(this instanceof fNOP
                 ? this
                 : oThis,
                 aArgs.concat(Array.prototype.slice.call(arguments)));
        };

    if (this.prototype) {
      // Function.prototype doesn't have a prototype property
      fNOP.prototype = this.prototype;
    }
    fBound.prototype = new fNOP();

    return fBound;
  };
}

utils = {
    parseJSON: function(json) {
        try {
            return JSON.parse(json);
        } catch(e) {
            return undefined;
        }
    },
    findSurfaceBelowPosition: function(pos) {
        var result = Entities.findRayIntersection({
            origin: pos,
            direction: { x: 0, y: -1, z: 0 }
        });
        if (result.intersects) {
            return result.intersection;
        }
        return pos;
    },
    formatNumberWithCommas: function(number) {
        return number + "";
        if (number === 0) {
            return "0";
        }

        var str = "";

        while (number > 0) {
            // Grab the digits in the 
            var belowThousand = number % 1000;
            if (str !== "") {
                str = "," + str;
            }
            str = belowThousand + str;
            number = Math.floor(number / 1000);
        }

        return str;
    }
};
