(function() {
    return {
        preload: function(uuid) {
            var shape = Entities.getEntityProperties(uuid).shape === 'Sphere' ? 'Cube' : 'Sphere';

            Entities.editEntity(uuid, {
                shape: shape,
                color: { red: 0xff, green: 0xff, blue: 0xff },
            });
        }
    };
})
