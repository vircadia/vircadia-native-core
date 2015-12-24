    var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));

    var whiteListBox = Entities.addEntity({
        type: "Box",
        color: {
            red: 10,
            green: 200,
            blue: 10
        },
        dimensions: {
            x: .2,
            y: .2,
            z: .2
        },
        position: center
    });

    var blackListBox = Entities.addEntity({
        type: "Box",
        color: {
            red: 100,
            green: 10,
            blue: 10
        },
        dimensions: {
            x: .2,
            y: .2,
            z: .2
        },
        position: Vec3.sum(center, {x: 0, y: .3, z: 0})
    });


    function castRay(event) {
        var pickRay = Camera.computePickRay(event.x, event.y);
        var pickResults = Entities.findRayIntersection(pickRay, true, [], [blackListBox]);

        if(pickResults.intersects) {
            print("INTERSECTION");
        }

    }

    function cleanup() {
        Entities.deleteEntity(whiteListBox);
        Entities.deleteEntity(blackListBox);
    }

    Script.scriptEnding.connect(cleanup);
    Controller.mousePressEvent.connect(castRay);