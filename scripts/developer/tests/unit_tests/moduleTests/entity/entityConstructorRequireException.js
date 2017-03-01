/* eslint-disable comma-dangle */
// test module-related exception from within "require" evaluation itself
(function() {
    var mod = Script.require('../exceptions/exception.js');
    return {
        preload: function(uuid) {
            print("entityConstructorRequireException::preload (never happens)", uuid, Script.resolvePath(''), mod);
        },
    };
});
