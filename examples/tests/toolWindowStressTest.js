var TOGGLE_RATE = 1000;
var RUNTIME = 60 * 1000;

var webView;
var running = true;

function toggleWindow() {
    if (!running) {
        return;
    }
    
    if (webView) {
        webView.close();
        webView = null;
    } else {
        webView = new OverlayWebWindow({
            title: 'Entity Properties', 
            source: "http://www.google.com", 
            toolWindow: true
        });
        webView.setVisible(true);
    }
    Script.setTimeout(toggleWindow, TOGGLE_RATE)
}

toggleWindow();
print("Creating window?")

Script.setTimeout(function(){
    print("Shutting down")
}, RUNTIME)
