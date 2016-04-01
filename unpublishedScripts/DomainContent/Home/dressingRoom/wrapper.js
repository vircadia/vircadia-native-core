 var TRANSFORMER_SCRIPT = Script.resolvePath('transformer.js');

 var AVATAR_COLLISION_HULL = 'atp:/dressingRoom/Avatar-Hull-4.obj';

 TransformerDoll = function(modelURL, spawnPosition, spawnRotation, dimensions) {
     print('SCRIPT REF AT TRANSFORMER CREATE::' + TRANSFORMER_SCRIPT);
     var transformerProps = {
         name: 'hifi-home-dressing-room-little-transformer',
         type: 'Model',
         shapeType: 'compound',
         compoundShapeURL: AVATAR_COLLISION_HULL,
         position: spawnPosition,
         rotation: Quat.fromPitchYawRollDegrees(spawnRotation.x, spawnRotation.y, spawnRotation.z),
         modelURL: modelURL,
         dynamic: true,
         gravity: {
             x: 0,
             y: -10,
             z: 0
         },
         visible: true,
         damping: 0.8,
         angularDamping: 0.8,
         userData: JSON.stringify({
             'grabbableKey': {
                 'grabbable': true
             },
             'hifiHomeTransformerKey': {
                 'basePosition': spawnPosition,
                 'baseRotation': Quat.fromPitchYawRollDegrees(spawnRotation.x, spawnRotation.y, spawnRotation.z),
             },
             'hifiHomeKey': {
                 'reset': true
             }
         }),
         density: 5000,
         dimensions: dimensions,
         script: TRANSFORMER_SCRIPT
     }
     var transformer = Entities.addEntity(transformerProps);

     print('CREATED TRANSFORMER' + transformer);

     return this;
 }