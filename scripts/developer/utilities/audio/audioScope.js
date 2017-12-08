var qml = Script.resourcesPath() + '/qml/AudioScope.qml';
var window = new OverlayWindow({
    title: 'Audio Scope',
    source: qml,
    width: 1200,
    height: 500
});
window.closed.connect(function () { 
	if (Audio.getRecording()) {
		Audio.stopRecording();
	}
	AudioScope.setVisible(false);
	AudioScope.setLocalEcho(false);
	AudioScope.setServerEcho(false);
	AudioScope.selectAudioScopeFiveFrames();
	Script.stop(); 
});