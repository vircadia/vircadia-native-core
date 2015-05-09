
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
    function TestFx() {
        var animationSettings = JSON.stringify({ fps: 30,
                                                 frameIndex: 0,
                                                 running: true,
                                                 firstFrame: 0,
                                                 lastFrame: 30,
                                                 loop: false });

        this.entity = Entities.addEntity({ type: "ParticleEffect",
                                           animationSettings: animationSettings,
                                           position: spawnPoint,
                                           textures: "http://www.hyperlogic.org/images/particle.png",
                                           emitRate: 30,
                                           emitStrength: 3,
                                           color: { red: 128, green: 255, blue: 255 },
                                           visible: true,
                                           locked: false });

        var self = this;
        this.timer = Script.setInterval(function () {
            print("AJT: setting animation props");
            var animProp = { animationFrameIndex: 0,
                             animationIsPlaying: true };
            Entities.editEntity(self.entity, animProp);
        }, 2000);

    }

    TestFx.prototype.Destroy = function () {
        Script.clearInterval(this.timer);
        Entities.editEntity(this.entity, { locked: false });
        Entities.deleteEntity(this.entity);
    }

    var objs = [];
    function Init() {
        objs.push(new TestBox());
        objs.push(new TestFx());
    }

    function ShutDown() {
        var i, len = entities.length;
        for (i = 0; i < len; i++) {
            objs[i].Destroy();
        }
        objs = [];
    }

    Init();
    Script.scriptEnding.connect(ShutDown);
})();


