// test dual-purpose module and standalone Entity script
function MyEntity(filename) {
    return {
        preload: function(uuid) {
            print("entityConstructorModule.js::preload");
            if (typeof module === 'object') {
                print("module.filename", module.filename);
                print("module.parent.filename", module.parent && module.parent.filename);
            }
        },
        clickDownOnEntity: function(uuid, evt) {
            print("entityConstructorModule.js::clickDownOnEntity");
        },
    };
}

try {
    module.exports = MyEntity;
} catch(e) {}
print('entityConstructorModule::MyEntity', typeof MyEntity);
(MyEntity)
