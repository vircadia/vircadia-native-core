(function() {
    return {
        preload: function(uuid) {
            var props = Entities.getEntityProperties(uuid);
            var shape = props.shape === 'Sphere' ? 'Hexagon' : 'Sphere';

            Entities.editEntity(uuid, {
                shape: shape,
                color: { red: 0xff, green: 0xff, blue: 0xff },
            });
            this.name = props.name;
            print("preload", this.name);
        },
        unload: function(uuid) {
            print("unload", this.name);
            Entities.editEntity(uuid, {
                color: { red: 0x0f, green: 0x0f, blue: 0xff },
            });
        },
    };
})
