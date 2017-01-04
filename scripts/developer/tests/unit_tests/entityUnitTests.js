describe('Entity', function() {
    var center = Vec3.sum(
        MyAvatar.position,
        Vec3.multiply(3, Quat.getFront(Camera.getOrientation()))
    );
    var boxEntity;
    var boxProps = {
        type: 'Box',
        color: {
            red: 255,
            green: 255,
            blue: 255,
        },
        position: center,
        dimensions: {
            x: 1,
            y: 1,
            z: 1,
        },
    };

    beforeEach(function() {
        boxEntity = Entities.addEntity(boxProps);
    });

    afterEach(function() {
        Entities.deleteEntity(boxEntity);
        boxEntity = null;
    });

    it('can be added programmatically', function() {
        expect(typeof(boxEntity)).toEqual('string');
    });

    it('instantiates itself correctly', function() {
        var props = Entities.getEntityProperties(boxEntity);
        expect(props.type).toEqual(boxProps.type);
    });

    it('can be modified after creation', function() {
        var newPos = {
            x: boxProps.position.x,
            y: boxProps.position.y,
            z: boxProps.position.z + 1.0,
        };
        Entities.editEntity(boxEntity, {
            position: newPos,
        });

        var props = Entities.getEntityProperties(boxEntity);
        expect(Math.round(props.position.z)).toEqual(Math.round(newPos.z));
    });

    it("\'s last edited property working correctly", function() {
        var props = Entities.getEntityProperties(boxEntity);
        expect(props.lastEdited).toBeDefined();
        expect(props.lastEdited).not.toBe(0);
        var prevLastEdited = props.lastEdited;

        // Now we make an edit to the entity, which should update its last edited time
        Entities.editEntity(boxEntity, {color: {red: 0, green: 255, blue: 0}});
        props = Entities.getEntityProperties(boxEntity);
        expect(props.lastEdited).toBeGreaterThan(prevLastEdited);
    });
});