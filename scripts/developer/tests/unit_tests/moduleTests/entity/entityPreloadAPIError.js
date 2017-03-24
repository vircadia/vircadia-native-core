/* eslint-disable comma-dangle */
// test module method exception being thrown within preload
(function() {
    var apiMethod = Script.require('../exceptions/exceptionInFunction.js');
    print(Script.resolvePath(''), "apiMethod", apiMethod);
    return {
        preload: function(uuid) {
            // this next line throws from within apiMethod
            print(apiMethod());
            print("entityPreloadAPIException::preload -- never seen --", uuid, Script.resolvePath(''));
        },
    };
});
