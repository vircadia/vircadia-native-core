
(function () {
    var spawnPoint = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));

    // constructor
    function TestBox() {
        this.entity = Entities.addEntity({ type: "Box",
                                           position: spawnPoint,
                                           dimentions: { x: 1, y: 1, z: 1 },
                                           color: { red: 100, green: 100, blue: 255 },
                                           gravity: { x: 0, y: 0, z: 0 },
                                           visible: true,
                                           locked: false,
                                           lifetime: 6000 });
        var self = this;
        this.timer = Script.setInterval(function () {
            var colorProp = { color: { red: Math.random() * 255,
                                       green: Math.random() * 255,
                                       blue: Math.random() * 255 } };
            Entities.editEntity(self.entity, colorProp);
        }, 1000);
    }

    TestBox.prototype.Destroy = function () {
        Script.clearInterval(this.timer);
        Entities.editEntity(this.entity, { locked: false });
        Entities.deleteEntity(this.entity);
    }

    // constructor
    function TestFx(color, emitDirection, emitRate, emitStrength, blinkRate) {
        var animationSettings = JSON.stringify({ fps: 30,
                                                 frameIndex: 0,
                                                 running: true,
                                                 firstFrame: 0,
                                                 lastFrame: 30,
                                                 loop: true });

        this.entity = Entities.addEntity({ type: "ParticleEffect",
                                           animationSettings: animationSettings,
                                           position: spawnPoint,
                                           dimensions: {x: 2, y: 2, z: 2},
                                           emitVelocity: {x: 0, y: 5, z: 0},
                                           velocitySpread: {x: 2, y: 0, z: 2},
                                           emitAcceleration: {x: 0, y: -9.8, z: 0},
                                           textures: "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png",
                                           color: color,
                                           lifespan: 1.0,
                                           visible: true,
                                           locked: false });

        this.isPlaying = true;

        var self = this;
        this.timer = Script.setInterval(function () {
            // flip is playing state
            self.isPlaying = !self.isPlaying;
            var animProp = { animationIsPlaying: self.isPlaying };
            Entities.editEntity(self.entity, animProp);
        }, (1 / blinkRate) * 1000);
    }

    TestFx.prototype.Destroy = function () {
        Script.clearInterval(this.timer);
        Entities.editEntity(this.entity, { locked: false });
        Entities.deleteEntity(this.entity);
    }

    var objs = [];
    function Init() {
        objs.push(new TestBox());
        objs.push(new TestFx({ red: 255, green: 0, blue: 0 },
                             { x: 0.5, y: 1.0, z: 0.0 },
                             100, 3, 1));
        objs.push(new TestFx({ red: 0, green: 255, blue: 0 },
                             { x: 0, y: 1, z: 0 },
                             1000, 5, 0.5));
        objs.push(new TestFx({ red: 0, green: 0, blue: 255 },
                             { x: -0.5, y: 1, z: 0 },
                             100, 3, 1));
    }

    function ShutDown() {
        var i, len = objs.length;
        for (i = 0; i < len; i++) {
            objs[i].Destroy();
        }
        objs = [];
    }

    Init();
    Script.scriptEnding.connect(ShutDown);
})();


