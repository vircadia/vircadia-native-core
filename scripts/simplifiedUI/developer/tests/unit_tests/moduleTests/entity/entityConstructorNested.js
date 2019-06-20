/* global module */
// test Entity constructor based on inherited constructor from a module
function constructor() {
    print("entityConstructorNested::constructor");
    var MyEntity = Script.require('./entityConstructorModule.js');
    return new MyEntity("-- created from entityConstructorNested --");
}

try {
    module.exports = constructor;
} catch (e) {
    constructor;
}

