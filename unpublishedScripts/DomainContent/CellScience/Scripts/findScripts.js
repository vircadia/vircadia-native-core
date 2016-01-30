var scriptName = "Controller";

function findScriptsInRadius(r) 
{
	var n = 0;
    var arrayFound = Entities.findEntities(MyAvatar.position, r);
    for (var i = 0; i < arrayFound.length; i++) {
		if (Entities.getEntityProperties(arrayFound[i]).script.indexOf(scriptName) != -1) {
            n++;
        }
    }
	print("found " + n + " copies of " + scriptName);
}

findScriptsInRadius(100000);