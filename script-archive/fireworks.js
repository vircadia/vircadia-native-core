var fireSound = SoundCache.getSound(Script.getExternalPath(Script.ExternalPaths.Assets, "sounds/Guns/GUN-SHOT2.raw"));
var audioOptions = {
  volume: 0.9,
  position: Vec3.sum(Camera.getPosition(), Quat.getFront(Camera.getOrientation()))
};

var DISTANCE_FROM_CAMERA = 7.0;

var bluePalette = [{
  red: 0,
  green: 206,
  blue: 209
}, {
  red: 173,
  green: 216,
  blue: 230
}, {
  red: 0,
  green: 191,
  blue: 255
}];

var greenPalette = [{
  red: 152,
  green: 251,
  blue: 152
}, {
  red: 127,
  green: 255,
  blue: 0
}, {
  red: 50,
  green: 205,
  blue: 50
}];

var redPalette = [{
  red: 255,
  green: 20,
  blue: 147
}, {
  red: 255,
  green: 69,
  blue: 0
}, {
  red: 255,
  green: 90,
  blue: 120
}];


var COLOR_RED = {red: 255, green: 0, blue: 0 };
var COLOR_GREEN = {red: 0, green: 255, blue: 0};
var COLOR_BLUE = {red: 0, green: 0, blue: 255};
var iconsX = 700;
var iconsY = 660;
var ICON_SIZE = 30;

var redIcon = Overlays.addOverlay("text", {
            backgroundColor: COLOR_RED,
            x: iconsX,
            y: iconsY,
            width: ICON_SIZE,
            height: ICON_SIZE,
            alpha: 0.0,
            backgroundAlpha: 1.0,
            visible: true
        });
var greenIcon = Overlays.addOverlay("text", {
            backgroundColor: COLOR_GREEN,
            x: iconsX + 50,
            y: iconsY,
            width: ICON_SIZE,
            height: ICON_SIZE,
            alpha: 0.0,
            backgroundAlpha: 1.0,
            visible: true
        });
var blueIcon = Overlays.addOverlay("text", {
            backgroundColor: COLOR_BLUE,
            x: iconsX + 100,
            y: iconsY,
            width: ICON_SIZE,
            height: ICON_SIZE,
            alpha: 0.0,
            backgroundAlpha: 1.0,
            visible: true
        });


var NUM_BURSTS = 11;
var SPEED = 6.0;

var rockets = [];
Rocket = function(point, colorPalette) {
  //default to blue palette if no palette passed in
  this.colors = colorPalette;
  this.point = point;
  this.bursts = [];
  this.burst = false;

  this.emitRate = randInt(80, 120);

  this.rocket = Entities.addEntity({
    type: "Sphere",
    position: this.point,
    dimensions: {
      x: 0.07,
      y: 0.07,
      z: 0.07
    },
    color: {
      red: 240,
      green: 240,
      blue: 240
    }
  });

  this.direction = {
    x: randFloat(-0.4, 0.4),
    y: 1.0,
    z: 0.0
  }

  this.time = 0.0;
  this.timeout = randInt(15, 40);
};

Rocket.prototype.update = function(deltaTime) {
  this.time++;

  Entities.editEntity(this.rocket, {
    velocity: Vec3.multiply(SPEED, this.direction)
  });
  var position = Entities.getEntityProperties(this.rocket).position;

  if (this.time > this.timeout) {
    this.explode(position);
    return;
  }
};


Rocket.prototype.explode = function(position) {
  Audio.playSound(fireSound, audioOptions);
  Entities.editEntity(this.rocket, {
    velocity: {
      x: 0,
      y: 0,
      z: 0
    }
  });

  var colorIndex = 0;
  var PI = 3.141593;
  var DEG_TO_RAD = PI / 180.0;

  for (var i = 0; i < NUM_BURSTS; ++i) {
    var color = this.colors[colorIndex];
    print(JSON.stringify(color));
    this.bursts.push(Entities.addEntity({
      type: "ParticleEffect",
      isEmitting: true,
      position: position,
      textures: 'https://raw.githubusercontent.com/ericrius1/SantasLair/santa/assets/smokeparticle.png',
      emitRate: this.emitRate,
      polarFinish: 25 * DEG_TO_RAD,
      color: color,
      lifespan: 1.0,
      visible: true,
      locked: false
    }));

    if (colorIndex < this.colors.length - 1) {
      colorIndex++;
    } 
  }

  this.burst = true;
  Entities.deleteEntity(this.rocket);
};

//var lastLoudness;
var LOUDNESS_RADIUS_RATIO = 10;

function update(deltaTime) {
  for (var i = 0; i < rockets.length; i++) {
    if (!rockets[i].burst) {
      rockets[i].update();
    }
  }
}

function randFloat(min, max) {
  return Math.random() * (max - min) + min;
}

function randInt(min, max) {
  return Math.floor(Math.random() * (max - min)) + min;
}

function computeWorldPoint(event) {
  var pickRay = Camera.computePickRay(event.x, event.y);
  var addVector = Vec3.multiply(Vec3.normalize(pickRay.direction), DISTANCE_FROM_CAMERA);
  return Vec3.sum(Camera.getPosition(), addVector);
}

function mousePressEvent(event) {
  var clickedOverlay = Overlays.getOverlayAtPoint({
            x: event.x,
            y: event.y
        });
  if(clickedOverlay === redIcon) {
    rockets.push(new Rocket(computeWorldPoint(event), redPalette));
  } else if (clickedOverlay === greenIcon) {
    rockets.push(new Rocket(computeWorldPoint(event), greenPalette));
  } else if (clickedOverlay === blueIcon) {
    rockets.push(new Rocket(computeWorldPoint(event), bluePalette));
  }
  
}

function cleanup() {
  Overlays.deleteOverlay(redIcon);
  Overlays.deleteOverlay(greenIcon);
  Overlays.deleteOverlay(blueIcon);
  for (var i = 0; i < rockets.length; ++i) {
    Entities.deleteEntity(rockets[i].rocket);
    for (var j = 0; j < NUM_BURSTS; ++j) {
      Entities.deleteEntity(rockets[i].bursts[j]);
    }
    
  }
}

Script.update.connect(update);
Script.scriptEnding.connect(cleanup);
Controller.mousePressEvent.connect(mousePressEvent);
