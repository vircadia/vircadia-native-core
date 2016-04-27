
LODManager.LODIncreased.connect(function() {
    print("LOD has been increased. You can now see " 
            + LODManager.getLODFeedbackText() 
            + ", fps:" + LODManager.getFPSAverage()
            + ", fast fps:" + LODManager.getFastFPSAverage()
        );
});

LODManager.LODDecreased.connect(function() {
    print("LOD has been decreased. You can now see " 
            + LODManager.getLODFeedbackText() 
            + ", fps:" + LODManager.getFPSAverage()
            + ", fast fps:" + LODManager.getFastFPSAverage()
        );
});