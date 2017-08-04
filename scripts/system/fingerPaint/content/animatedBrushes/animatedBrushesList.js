Script.include("animatedHueBrush.js");
Script.include("animatedRotationBrush.js");
Script.include("animatedTranslationBrush.js");

AnimatedBrushesInfo = {
    animatedHueBrush: {
        isEnabled: false,
        proto: AnimatedHueBrush.prototype,
        settings: null,
    },

    animatedRotationBrush: {
        isEnabled: false,
        proto: AnimatedRotationBrush.prototype,
        settings: null,
    },

    animatedTranslationBrush: {
        isEnabled: false,
        proto: AnimatedTranslationBrush.prototype,
        settings: null,
    },
}

animatedBrushFactory = function(animatedBrushName, settings, entityID) {
    switch (animatedBrushName) {
        case "animatedHueBrush":
            return new AnimatedHueBrush(settings, entityID);
        case "animatedRotationBrush":
            return new AnimatedRotationBrush(settings, entityID);
        case "animatedTranslationBrush":
            return new AnimatedTranslationBrush(settings, entityID);
        default:
            throw new Error("Invalid Brush Name. Could not instantiate " + animatedBrushName);
    }
}