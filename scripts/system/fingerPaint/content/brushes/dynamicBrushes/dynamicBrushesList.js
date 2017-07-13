Script.include("dynamicHueBrush.js");
Script.include("dynamicRotationBrush.js");
Script.include("dynamicTranslationBrush.js");

DynamicBrushesInfo = {
    DynamicHueBrush: {
    	isEnabled: false,
    	proto: DynamicHueBrush.prototype,
    	settings: null,
    },

    DynamicRotationBrush: {
    	isEnabled: false,
    	proto: DynamicRotationBrush.prototype,
    	settings: null,
    },

    DynamicTranslationBrush: {
    	isEnabled: false,
    	proto: DynamicTranslationBrush.prototype,
    	settings: null,
    },
}

dynamicBrushFactory = function(dynamicBrushName, settings) {
    switch (dynamicBrushName) {
    	case "DynamicHueBrush":
    		return new DynamicHueBrush(settings);
    	case "DynamicRotationBrush":
    		return new DynamicRotationBrush(settings);
    	case "DynamicTranslationBrush":
    		return new DynamicTranslationBrush(settings);
    	default:
    		throw new Error("Could not instantiate " + dynamicBrushName);
    }
}