function TransformerDoll(modelURL, spawnPosition, spawnRotation) {
    var transformerProps = {
        name: 'hifi-home-dressing-room-little-transformer',
        type: 'model',
        position: spawnPosition,
        rotation: spawnRotation,
        modelURL: modelURL,
        dynamic: true,
        gravity: {
            x: 0,
            y: -5,
            z: 0
        },
        userData: JSON.stringify({
            'grabbableKey': {
                'grabbable:', true
            },
            'hifiHomeTransformerKey': {
                'basePosition': spawnPosition
            },
            'hifiHomeKey': {
                'reset': true
            }
        }),
    }
    Entities.addEntity(transformerProps);
    return this;
}