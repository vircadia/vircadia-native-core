//
//  Created by Seiji Emery on 9/4/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("perfTest.js");

function makeEntity () {
    return Entities.addEntity({
        type: "Box",
        position: Vec3.sum(MyAvatar.position, {
            x: 0.0, y: 0.0, z: 1.0
        }),
        dimensions: { x: 0.1, y: 0.1, z: 0.1 },
        userData: "fooooooooooooooooooooooo"
    });
}

// Create + run tests
(function () {
    var entityTests = new TestRunner();

    entityTests.addTestCase('getEntityProperties')
        .before(function () {
            this.entity = makeEntity();
        })
        .after(function() {
            Entities.deleteEntity(this.entity);
        })
        .run(function() {
            var properties = Entities.getEntityProperties(this.entity);
            var foo = properties.userData;
        });

    entityTests.addTestCase('add + delete entity')
        .run(function() {
            var entity = makeEntity();
            Entities.deleteEntity(entity);
        });
    entityTests.addTestCase('update entity')
        .before(function () {
            this.entity = makeEntity();
        })
        .after(function () {
            Entities.deleteEntity(this.entity);
        })
        .run(function() {
            Entities.editEntity(this.entity, { userData: "barrrrrrr" });
        });

    TestCase.prototype.withEntityOp = function(op) {
        this.before(function(){
            this.entity = makeEntity();
        })
        this.after(function(){
            Entities.deleteEntity(this.entity);
        })
        this.run(op);
    }
    entityTests.addTestCase('find closest entity')
        .withEntityOp(function() {
            Entities.findClosestEntity(this.entity, MyAvatar.position, 100.0);
        })
    entityTests.addTestCase('findEntities')
        .withEntityOp(function(){
            Entities.findEntities(this.entity, MyAvatar.position, 10.0);
        })
    entityTests.addTestCase('findEntitiesInBox')
        .withEntityOp(function(){
            Entities.findEntitiesInBox(this.entity, MyAvatar.position, {x: 10.0, y: 10.0, z: 10.0});
        })

    TestCase.prototype.withRay = function(op) {
        this.before(function(){
            this.ray = Camera.computePickRay(500, 200);
        });
        this.run(op);
    }
    entityTests.addTestCase('findRayIntersection, precisionPicking=true')
        .withRay(function(){
            Entities.findRayIntersection(this.ray, true);
        })
    entityTests.addTestCase('findRayIntersection, precisionPicking=false')
        .withRay(function(){
            Entities.findRayIntersection(this.ray, false);
        });
    entityTests.addTestCase('findRayIntersectionBlocking, precisionPicking=true')
        .withRay(function(){
            Entities.findRayIntersectionBlocking(this.ray, true);
        })
    entityTests.addTestCase('findRayIntersectionBlocking, precisionPicking=false')
        .withRay(function(){
            Entities.findRayIntersectionBlocking(this.ray, false);
        })
    entityTests.addTestCase('no-op')
        .run(function(){});

    print("Running entity tests");
    entityTests.runAllTestsWithIterations([10, 100, 1000], 1e3, 10);
    entityTests.writeResultsToLog();
})();



