(function(){ 
    this.enterEntity = function(entityID) {
        var properties = Entities.getEntityProperties(entityID, 'userData');

        if (properties && properties.userData !== undefined) {
            portalDestination = properties.userData;

            print("portal.js | enterEntity() .... The portal destination is " + portalDestination);

            if (portalDestination.length > 0) {
                var destination;
                if (portalDestination[0] == '/') {
                    destination = portalDestination;
                } else {
                    destination = "hifi://" + portalDestination;
                }
                print("Teleporting to " + destination);
                Window.location = destination;
            }
        }
    }; 
});
