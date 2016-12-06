(function() {
    return {
        preload: function(entityID) {
            Entities.editEntity(entityID, {
                localRenderAlpha: 0.2
            });
        }
    };
});
