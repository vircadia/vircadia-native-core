/* eslint-disable comma-dangle */
// test requiring a module from within preload
(function constructor() {
    return {
        preload: function(uuid) {
            print("entityPreloadRequire::preload");
            var example = Script.require('../example.json');
            print("entityPreloadRequire::example::name", example.name);
        },
    };
});
