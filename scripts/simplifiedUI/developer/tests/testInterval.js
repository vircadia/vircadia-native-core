// Tester, try testing these different settings.
//
// Changing the TIMER_HZ will show different performance results. It's expected that at 90hz you'll see a fair
// amount of variance, as Qt Timers simply aren't accurate enough. In general RPC peformance should match the timer
// without significant difference in variance.

var TIMER_HZ = 50; // Change this for different values
var TIMER_INTERVAL = 1000 / TIMER_HZ;
var TIMER_WORK_EFFORT = 0; // 1000 is light work, 1000000 ~= 30ms

var UPDATE_HZ = 60; // standard script update rate
var UPDATE_INTERVAL = 1000/UPDATE_HZ; // standard script update interval
var UPDATE_WORK_EFFORT = 0; // 1000 is light work, 1000000 ~= 30ms

var basePosition = Vec3.sum(Camera.getPosition(), Quat.getForward(Camera.getOrientation()));

var timerBox = Entities.addEntity(
  { type: "Box",
    position: basePosition,
    dimensions: { x: 0.1, y: 0.1, z: 0.1 },  
    color: { red: 255, green: 0, blue: 255 },
    dynamic: false,
    collisionless: true
  });

var lastTick = Date.now();
var deltaTick = 0;
var tickSamples = 0;
var totalVariance = 0;
var tickCount = 0;
var totalWork = 0;
var highVarianceCount = 0;
var varianceCount = 0;

print("Set interval = " + TIMER_INTERVAL);

var timerTime = 0.0; 
var range = 0.5;
var rotationFactor = 0.5; // smaller == faster
var omega = 2.0 * Math.PI / rotationFactor;

var ticker = Script.setInterval(function() {

	tickCount++;
	var tickNow = Date.now();
	deltaTick = tickNow - lastTick;

    var variance = Math.abs(deltaTick - TIMER_INTERVAL);
    totalVariance += variance;

    if (variance > 1) {
        varianceCount++;
    }
    if (variance > 5) {
        highVarianceCount++;
    }

	var preWork = Date.now();
    var y = 2;
    for (var x = 0; x < TIMER_WORK_EFFORT; x++) {
        y = y * y;
    }
	var postWork = Date.now();
    deltaWork = postWork - preWork;
    totalWork += deltaWork;

    // move a box
    var deltaTime = deltaTick / 1000;
    timerTime += deltaTime;
    rotation = Quat.angleAxis(timerTime * omega  / Math.PI * 180.0, { x: 0, y: 1, z: 0 });
    Entities.editEntity(timerBox, 
                {
                    position: { x: basePosition.x + Math.sin(timerTime * omega) / 2.0 * range, 
                                y: basePosition.y, 
                                z: basePosition.z  },  
                    //rotation: rotation 
                }); 

	tickSamples = tickSamples + deltaTick;
    lastTick = tickNow;

    // report about every 5 seconds
	if(tickCount == (TIMER_HZ * 5)) {
		print("TIMER  -- For " + tickCount + " samples average interval = " + tickSamples/tickCount + " ms" 
                     + " average variance:" + totalVariance/tickCount + " ms"
                     + " min variance:" + varianceCount + " [" + (varianceCount/tickCount) * 100 + " %] "
                     + " high variance:" + highVarianceCount + " [" + (highVarianceCount/tickCount) * 100 + " %] "
                     + " average work:" + totalWork/tickCount + " ms");
        tickCount = 0;
        tickSamples = 0;
        totalWork = 0;
        totalVariance = 0;
        varianceCount = 0;
        highVarianceCount = 0;
	}
}, TIMER_INTERVAL);

/////////////////////////////////////////////////////////////////////////

var rpcPosition = Vec3.sum(basePosition, { x:0, y: 0.2, z: 0});

var theRpcFunctionInclude = Script.resolvePath("testIntervalRpcFunction.js");

print("theRpcFunctionInclude:" + theRpcFunctionInclude);

var rpcBox = Entities.addEntity(
  { type: "Box",
    position: rpcPosition,
    dimensions: { x: 0.1, y: 0.1, z: 0.1 },  
    color: { red: 255, green: 255, blue: 0 },
    dynamic: false,
    collisionless: true,
    script: theRpcFunctionInclude
  });

var rpcLastTick = Date.now();
var rpcTotalTicks = 0;
var rpcCount = 0;
var rpcTotalVariance = 0;
var rpcTickCount = 0;
var rpcVarianceCount = 0;
var rpcHighVarianceCount = 0;

var rpcTicker = Script.setInterval(function() {

	rpcTickCount++;
	var tickNow = Date.now();
	var deltaTick = tickNow - rpcLastTick;
    var variance = Math.abs(deltaTick - TIMER_INTERVAL);
    rpcTotalVariance += variance;

    if (variance > 1) {
        rpcVarianceCount++;
    }
    if (variance > 5) {
        rpcHighVarianceCount++;
    }

	rpcTotalTicks += deltaTick;
    rpcLastTick = tickNow;

    var args = [range, rotationFactor, omega, TIMER_INTERVAL, TIMER_HZ];

    Entities.callEntityMethod(rpcBox, "doRPC", args);

    // report about every 5 seconds
	if(rpcTickCount == (TIMER_HZ * 5)) {
		print("RPCTIMER- For " + rpcTickCount + " samples average interval = " + rpcTotalTicks/rpcTickCount + " ms" 
                     + " average variance:" + rpcTotalVariance/rpcTickCount + " ms"
                     + " min variance:" + rpcVarianceCount + " [" + (rpcVarianceCount/rpcTickCount) * 100 + " %] "
                     + " high variance:" + rpcHighVarianceCount + " [" + (rpcHighVarianceCount/rpcTickCount) * 100 + " %] "
              );
        rpcTickCount = 0;
        rpcTotalTicks = 0;
        rpcTotalVariance = 0;
        rpcVarianceCount = 0;
        rpcHighVarianceCount = 0;
	}
}, TIMER_INTERVAL);


var updateCount = 0;
var updateTotalElapsed = 0;
var lastUpdate = Date.now();
var updateTotalWork = 0;
var updateTotalVariance = 0;
var updateVarianceCount = 0;
var updateHighVarianceCount = 0;

var updatePosition = Vec3.sum(basePosition, { x:0, y: -0.2, z: 0});

var updateBox = Entities.addEntity(
  { type: "Box",
    position: updatePosition,
    dimensions: { x: 0.1, y: 0.1, z: 0.1 },  
    color: { red: 0, green: 255, blue: 255 },
    dynamic: false,
    collisionless: true
  });


var updateTime = 0;
var updateFunction = function(deltaTime){
	updateCount++;
    var updateAt = Date.now();
	deltaUpdate = updateAt - lastUpdate;
	updateTotalElapsed += deltaUpdate;
	lastUpdate = updateAt;

    var variance = Math.abs(deltaUpdate - UPDATE_INTERVAL);
    updateTotalVariance += variance;

    if (variance > 1) {
        updateVarianceCount++;
    }

    if (variance > 5) {
        updateHighVarianceCount++;
    }

	var preWork = Date.now();
    var y = 2;
    for (var x = 0; x < UPDATE_WORK_EFFORT; x++) {
        y = y * y;
    }
	var postWork = Date.now();
    deltaWork = postWork - preWork;
    updateTotalWork += deltaWork;

    // move a box

    updateTime += deltaTime;
    rotation = Quat.angleAxis(updateTime * omega  / Math.PI * 180.0, { x: 0, y: 1, z: 0 });
    Entities.editEntity(updateBox, 
                {
                    position: { x: updatePosition.x + Math.sin(updateTime * omega) / 2.0 * range, 
                                y: updatePosition.y, 
                                z: updatePosition.z  },  
                }); 


	if(updateCount == (UPDATE_HZ * 5)) {
		print("UPDATE -- For " + updateCount + " samples average update = " + updateTotalElapsed/updateCount + " ms" 
                     + " average variance:" + updateTotalVariance/updateCount + " ms"
                     + " min variance:" + updateVarianceCount + " [" + (updateVarianceCount/updateCount) * 100 + " %] "
                     + " high variance:" + updateHighVarianceCount + " [" + (updateHighVarianceCount/updateCount) * 100 + " %] "
                     + " average work:" + updateTotalWork/updateCount + " ms");

        updateCount = 0;
        updateTotalElapsed = 0;
        updateTotalWork = 0;
        updateTotalVariance = 0;
        updateVarianceCount = 0;
        updateHighVarianceCount = 0;
	}
};

Script.update.connect(updateFunction);

Script.scriptEnding.connect(function(){
    Entities.deleteEntity(timerBox);
    Entities.deleteEntity(rpcBox);
    Entities.deleteEntity(updateBox);
});
