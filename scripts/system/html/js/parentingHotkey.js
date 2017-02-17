var keyReleaseEvent = function (event) {
    if (event.text === 'p' && event.isControl && !event.isAutoRepeat ) {
        if (event.isShifted) {
            EventBridge.emitWebEvent(JSON.stringify({ type: 'unparent' }));
        } else {
            EventBridge.emitWebEvent(JSON.stringify({ type: 'parent' }));
        }
    }
};

window.onkeypress = keyReleaseEvent;
Controller.keyReleaseEvent.connect(keyReleaseEvent);
