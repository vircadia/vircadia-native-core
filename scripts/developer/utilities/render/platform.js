// Test key commands
PlatformInfo.getComputer()
// {"OS":"WINDOWS","keys":null,"model":"","profileTier":"HIGH","vendor":""}
PlatformInfo.getNumCPUs()
// 1
PlatformInfo.getCPU(0)
//{"clockSpeed":" 4.00GHz","model":") i7-6700K CPU @ 4.00GHz","numCores":8,"vendor":"Intel(R) Core(TM) i7-6700K CPU @ 4.00GHz"}
PlatformInfo.getNumGPUs()
// 1
PlatformInfo.getGPU(0)
// {"driver":"25.21.14.1967","model":"NVIDIA GeForce GTX 1080","vendor":"NVIDIA GeForce GTX 1080","videoMemory":8079}

var window = Desktop.createWindow(Script.resolvePath('./luci/Platform.qml'), {
    title: "Platform",
    presentationMode: Desktop.PresentationMode.NATIVE,
    size: {x: 350, y: 700}
});

