(function(){
    
    var self = this;
    
    this.preload = function(entityId) {

        this.entityId = entityId;
        this.updateInterval = 30;
        this.frame = 0;
        
        this.minVelocity = 0.15;
        this.maxVelocity = 0.35;
        this.minAngularVelocity = 0.3;
        this.maxAngularVelocity = 0.7;

    }
    
    this.update = function(deltaTime) {
        
        self.frame++;
        
        if (self.frame > self.updateInterval) {
            
            self.updateInterval = 20 * Math.random() + 0;
            self.frame = 0;

            var magnitudeV = (self.maxVelocity - self.minVelocity) * Math.random() + self.minVelocity;
            var magnitudeAV = (self.maxAngularVelocity - self.minAngularVelocity) * Math.random() + self.minAngularVelocity;
            var directionV = {x: Math.random() - 0.5, y: Math.random() - 0.5, z: Math.random() - 0.5};
            var directionAV = {x: Math.random() - 0.5, y: Math.random() - 0.5, z: Math.random() - 0.5};
            
            Entities.editEntity(self.entityId, {
                velocity: Vec3.multiply(magnitudeV, Vec3.normalize(directionV)),
                angularVelocity: Vec3.multiply(magnitudeAV, Vec3.normalize(directionAV))
            });
            
        }
        
    }
    
    this.unload = function() {
        
        Script.update.disconnect(this.update);
        
    }

    Script.update.connect(this.update);
    
})