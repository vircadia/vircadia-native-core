var entities = Entities.findEntities(MyAvatar.position, 10000);
var URL = "https://s3.amazonaws.com/hifi-public/models/props/chess/";

for(var i in entities) {
  if (Entities.getEntityProperties(entities[i]).modelURL.slice(0, URL.length) === URL) {
    Entities.deleteEntity(entities[i]);
  }
} 