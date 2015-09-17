FireManager = function() {

    this.reset = function() {
        if (!this.fire) {
            var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));
            var ZERO_VEC = {
                x: 0,
                y: 0,
                z: 0
            }
            var totalTime = 0;
            var FIRE_COLOR = {
                red: 255,
                green: 255,
                blue: 255
            };
            var minLightIntensity = 3;
            var maxLightIntensity = 11;

            var minTimeFactor = .1;
            var maxTimeFactor = 1;

            var LIGHT_COLOR = {
                red: 255,
                green: 100,
                blue: 28
            }

            var animationSettings = JSON.stringify({
                fps: 30,
                running: true,
                loop: true,
                firstFrame: 1,
                lastFrame: 10000
            });

            var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));

            this.fire = Entities.addEntity({
                type: "ParticleEffect",
                animationSettings: animationSettings,
                textures: "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png",
                position: {
                    x: 551.5435791015625,
                    y: 494.87728881835938,
                    z: 502.01531982421875
                },
                name: "fire",
                emitRate: 100,
                colorStart: {
                    red: 46,
                    green: 39,
                    blue: 137
                },
                color: {
                    red: 200,
                    green: 99,
                    blue: 42
                },
                colorFinish: {
                    red: 255,
                    green: 99,
                    blue: 32
                },
                emitVelocity: {
                    x: .0,
                    y: 0.1,
                    z: 0
                },

                velocitySpread: {
                    x: .1,
                    y: .01,
                    z: .1
                },
                radiusSpread: .1,
                radiusStart: .1,
                particleRadius: .05,
                radiusFinish: 0.01,

                alphaStart: 0.5,
                alpha: 1,
                alphaFinish: 0.0,
                emitAcceleration: {
                    x: 0.1,
                    y: 1,
                    z: .0
                },
                accelerationSpread: {
                    x: .01,
                    y: .1,
                    z: .01
                },
                lifespan: 2
            });

        } else {
            Entities.deleteEntity(this.fire);
            this.fire = null;
        }

    }
}