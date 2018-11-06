/* eslint-disable comma-dangle */
// test module method exception being thrown within main constructor
(function() {
    var apiMethod = Script.require('../exceptions/exceptionInFunction.js');
    print("apiMethod", apiMethod);
    // this next line throws from within apiMethod
    print(apiMethod());
    return {
        preload: function(uuid) {
            print("entityConstructorAPIException::preload -- never seen --", uuid);
        },
    };
});
