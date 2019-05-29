// Example of using a "system module" to decouple Vec3's implementation details.
//
// Users would bring Vec3 support in as a module:
//   var vec3 = Script.require('vec3');
//

// (this example is compatible with using as a Script.include and as a Script.require module)
try {
    // Script.require
    module.exports = vec3;
} catch(e) {
    // Script.include
    Script.registerValue("vec3", vec3);
}

vec3.fromObject = function(v) {
    //return new vec3(v.x, v.y, v.z);
    //... this is even faster and achieves the same effect
    v.__proto__ = vec3.prototype;
    return v;
};

vec3.prototype = {
    multiply: function(v2) {
        // later on could support overrides like so:
        //     if (v2 instanceof quat) { [...] }
        // which of the below is faster (C++ or JS)?
        // (dunno -- but could systematically find out and go with that version)

        // pure JS option
        // return new vec3(this.x * v2.x, this.y * v2.y, this.z * v2.z);

        // hybrid C++ option
        return vec3.fromObject(Vec3.multiply(this, v2));
    },
    // detects any NaN and Infinity values
    isValid: function() {
        return isFinite(this.x) && isFinite(this.y) && isFinite(this.z);
    },
    // format Vec3's, eg:
    //     var v = vec3();
    //     print(v); // outputs [Vec3 (0.000, 0.000, 0.000)]
    toString: function() {
        if (this === vec3.prototype) {
            return "{Vec3 prototype}";
        }
        function fixed(n) { return n.toFixed(3); }
        return "[Vec3 (" + [this.x, this.y, this.z].map(fixed) + ")]";
    },
};

vec3.DEBUG = true;

function vec3(x, y, z) {
    if (!(this instanceof vec3)) {
        // if vec3 is called as a function then re-invoke as a constructor
        // (so that `value instanceof vec3` holds true for created values)
        return new vec3(x, y, z);
    }

    // unfold default arguments (vec3(), vec3(.5), vec3(0,1), etc.)
    this.x = x !== undefined ? x : 0;
    this.y = y !== undefined ? y : this.x;
    this.z = z !== undefined ? z : this.y;

    if (vec3.DEBUG && !this.isValid())
        throw new Error('vec3() -- invalid initial values ['+[].slice.call(arguments)+']');
};

