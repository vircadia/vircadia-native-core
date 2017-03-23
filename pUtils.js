getEntityTextures = function(id) {
    var results = null;
    var properties = Entities.getEntityProperties(id, "textures");
    if (properties.textures) { 
        try {
            results = JSON.parse(properties.textures);
        } catch(err) {
            logDebug(err);
            logDebug(properties.textures);
        }
    }
    return results ? results : {};
}

setEntityTextures = function(id, textureList) {
	var json = JSON.stringify(textureList);
	Entities.editEntity(id, {textures: json});
}

editEntityTextures = function(id, textureName, textureURL) {
	var textureList = getEntityTextures(id);
	textureList[textureName] = textureURL;
	setEntityTextures(id, textureList);
}