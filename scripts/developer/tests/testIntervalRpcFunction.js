(function() { 
    var x = false;
    var y = false;
    var z = false;

    var entityLastTick = Date.now();
    var entityTotalTicks = 0;
    var entityCount = 0;
    var entityTotalVariance = 0;
    var entityTickCount = 0;
    var entityVarianceCount = 0;
    var entityHighVarianceCount = 0;
    var _entityID;
    var time = 0;

    function Foo() { return; };
    Foo.prototype = {
        preload: function(entityID) { print('Foo preload'); _entityID = entityID; },
        doRPC: function(entityID, args) { 
            var range = args[0];
            var rotationFactor = args[1];
            var omega = args[2];
            var TIMER_INTERVAL = args[3];
            var TIMER_HZ = args[4];

            // first time, set our x,y,z
            if (x === false) {
                var position = Entities.getEntityProperties(_entityID, "position").position;
                x = position.x;
                y = position.y;
                z = position.z;
            }
	        entityTickCount++;
	        var tickNow = Date.now();
	        var deltaTick = tickNow - entityLastTick;
            var variance = Math.abs(deltaTick - TIMER_INTERVAL);
            entityTotalVariance += variance;

            if (variance > 1) {
                entityVarianceCount++;
            }
            if (variance > 5) {
                entityHighVarianceCount++;
            }

	        entityTotalTicks += deltaTick;
            entityLastTick = tickNow;

            // move self!!
            var deltaTime = deltaTick / 1000;
            time += deltaTime;
            rotation = Quat.angleAxis(time * omega  / Math.PI * 180.0, { x: 0, y: 1, z: 0 });
            Entities.editEntity(_entityID, 
                        {
                            position: { x: x + Math.sin(time * omega) / 2.0 * range, 
                                        y: y, 
                                        z: z  },  
                        }); 


	        if(entityTickCount == (TIMER_HZ * 5)) {
		        print("ENTITY -- For " + entityTickCount + " samples average interval = " + entityTotalTicks/entityTickCount + " ms" 
                             + " average variance:" + entityTotalVariance/entityTickCount + " ms"
                             + " min variance:" + entityVarianceCount + " [" + (entityVarianceCount/entityTickCount) * 100 + " %] "
                             + " high variance:" + entityHighVarianceCount + " [" + (entityHighVarianceCount/entityTickCount) * 100 + " %] "
                      );
                entityTickCount = 0;
                entityTotalTicks = 0;
                entityTotalVariance = 0;
                entityVarianceCount = 0;
                entityHighVarianceCount = 0;
	        }
        }
    }; 

    return new Foo(); 
});
