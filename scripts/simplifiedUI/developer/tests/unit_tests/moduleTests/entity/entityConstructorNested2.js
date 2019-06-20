/* global module */
// test Entity constructor based on nested, inherited module constructors
function constructor() {
    print("entityConstructorNested2::constructor");

    // inherit from entityConstructorNested
    var MyEntity = Script.require('./entityConstructorNested.js');
    function SubEntity() {}
    SubEntity.prototype = new MyEntity('-- created from entityConstructorNested2 --');

    // create new instance
    var entity = new SubEntity();
    // "override" clickDownOnEntity for just this new instance
    entity.clickDownOnEntity = function(uuid, evt) {
        print("entityConstructorNested2::clickDownOnEntity");
        SubEntity.prototype.clickDownOnEntity.apply(this, arguments);
    };
    return entity;
}

try {
    module.exports = constructor;
} catch (e) {
    constructor;
}
