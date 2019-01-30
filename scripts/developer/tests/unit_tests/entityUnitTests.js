/* eslint-env jasmine */

describe('Entity', function() {
    var center = Vec3.sum(
        MyAvatar.position,
        Vec3.multiply(3, Quat.getForward(Camera.getOrientation()))
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

    it('serversExist', function() {
        expect(Entities.serversExist()).toBe(true);
    });

    it('canRezTmp', function() {
        expect(Entities.canRezTmp()).toBe(true);
    });

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

    it("filtering getEntityProperties seems to work correctly", function() {
        var properties = Entities.getEntityProperties(boxEntity, 'position');
        expect(properties.id).toBe(boxEntity);
        expect(properties.type).toBe(boxProps.type);
        expect(properties.position).toBeDefined();
    });

    it("filtering getMultipleEntityProperties seems to work correctly", function () {
        var sphereProps = {
            type: 'Sphere',
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
        var sphereEntity = Entities.addEntity(sphereProps);

        var entityIDs = [boxEntity, sphereEntity];
        var filterTypes = 'type';
        var propertySets = Entities.getMultipleEntityProperties(entityIDs, filterTypes);
        expect(propertySets.length).toBe(entityIDs.length);
        expect(propertySets[0].type).toBe(boxProps.type);
        expect(Object.keys(propertySets[0]).length).toBe(1);
        expect(propertySets[1].type).toBe(sphereProps.type);
        expect(Object.keys(propertySets[1]).length).toBe(1);

        filterTypes = ['id', 'type'];
        propertySets = Entities.getMultipleEntityProperties(entityIDs, filterTypes);
        expect(propertySets.length).toBe(entityIDs.length);
        expect(Object.keys(propertySets[0]).length).toBe(filterTypes.length);
        expect(propertySets[0].id).toBe(boxEntity);
        expect(propertySets[0].type).toBe(boxProps.type);
        expect(Object.keys(propertySets[1]).length).toBe(filterTypes.length);
        expect(propertySets[1].id).toBe(sphereEntity);
        expect(propertySets[1].type).toBe(sphereProps.type);

        Entities.deleteEntity(sphereEntity);
    });

});
