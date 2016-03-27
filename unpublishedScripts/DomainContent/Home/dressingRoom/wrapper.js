 var TRANSFORMER_SCRIPT = Script.resolvePath('transformer.js?' + Math.random());
 TransformerDoll = function(modelURL, spawnPosition, spawnRotation) {

     var transformerProps = {
         name: 'hifi-home-dressing-room-little-transformer',
         type: 'Model',
         shapeType: 'box',
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
                 'grabbable': true
             },
             'hifiHomeTransformerKey': {
                 'basePosition': spawnPosition
             },
             'hifiHomeKey': {
                 'reset': true
             }
         }),
         script: TRANSFORMER_SCRIPT
     }
     var transformer = Entities.addEntity(transformerProps);
     Script.setTimeout(function() {
         var actualProps = Entities.getEntityProperties(transformer);
         var quarterSize = Vec3.multiply(0.25, actualProps.naturalDimensions);
         Entities.editEntity(transformer, {
             dimensions: quarterSize
         });
     }, 1500)

     print('CREATED TRANSFORMER' + transformer);
     print('at location: ' + JSON.stirngify(transformerProps.position))

     return this;
 }