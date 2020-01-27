'use strict'

import { app, protocol, BrowserWindow } from 'electron'
import {
	installVueDevtools,
	createProtocol,
} from 'vue-cli-plugin-electron-builder/lib'
import path from 'path'
const isDevelopment = process.env.NODE_ENV !== 'production'
const storage = require('electron-json-storage');
const { shell } = require('electron')
const electronDl = require('electron-dl');
const { readdirSync } = require('fs')
const { forEach } = require('p-iteration');
const fs = require('fs');

var glob = require('glob');
 
electronDl();

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the JavaScript object is garbage collected.
let win

// Scheme must be registered before the app is ready
protocol.registerSchemesAsPrivileged([{scheme: 'app', privileges: { secure: true, standard: true } }])

function createWindow () {
	// Create the browser window.
	win = new BrowserWindow({ 
		width: 1000, 
		height: 800, 
		icon: path.join(__static, '/resources/launcher.ico'), 
		resizable: false,
		webPreferences: {
			nodeIntegration: true,
			devTools: true
		} 
	})

	// This line disables the default menu behavior on Windows.
	win.setMenu(null);

	if (process.env.WEBPACK_DEV_SERVER_URL) {
		// Load the url of the dev server if in development mode
		win.loadURL(process.env.WEBPACK_DEV_SERVER_URL)
	if (!process.env.IS_TEST) win.webContents.openDevTools()
	} else {
		createProtocol('app')
		// Load the index.html when not in development
		win.loadURL('app://./index.html')
	}

	win.on('closed', () => {
	win = null
	})
}

// Quit when all windows are closed.
app.on('window-all-closed', () => {
	// On macOS it is common for applications and their menu bar
	// to stay active until the user quits explicitly with Cmd + Q
	if (process.platform !== 'darwin') {
		app.quit()
	}
})

app.on('activate', () => {
	// On macOS it's common to re-create a window in the app when the
	// dock icon is clicked and there are no other windows open.
	if (win === null) {
		createWindow()
	}
})

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.on('ready', async () => {
  if (isDevelopment && !process.env.IS_TEST) {
    // Install Vue Devtools
    try {
		// console.info("Installing VueDevTools, if this does not work, Electron will not generate an interface.");
		// await installVueDevtools()
	} catch (e) {
		// console.error('Vue Devtools failed to install:', e.toString())
	}

  }
  createWindow();
})

// Exit cleanly on request from parent process in development mode.
if (isDevelopment) {
  if (process.platform === 'win32') {
    process.on('message', data => {
      if (data === 'graceful-exit') {
        app.quit()
      }
    })
  } else {
    process.on('SIGTERM', () => {
      app.quit()
    })
  }
}

// Settings.JSON bootstrapping
// storage.setDataPath(app.getAppPath() + "/settings");

console.log("Data Path: " + storage.getDataPath());

// if(process.env.LAST_ATHENA_INSTALLED) {
//   var lastInstalled = process.env.LAST_ATHENA_INSTALLED;
//   storage.set('athena_interface.location', lastInstalled, function(error) {
//     if (error) throw error;
//   });
// }



var storagePath = {
	default: storage.getDefaultDataPath(),
	interface: null,
	interfaceSettings: null,
	currentLibrary: null,
};

// We are going to default the library to storage.getDefaultDataPath().
setLibrary(storagePath.default);

// shell.openItem(storagePath.default);

var requireInterfaceSelection;

async function generateInterfaceList(interfaces) {
  var interfacesArray = [];
  var dataPath;
  for (var i in interfaces) {
    var client = interfaces[i];
    // dataPath = client + "launcher_settings";
	dataPath = client;

    // await getSetting('interface_package', dataPath).then(function(pkg){
	// 	var appName = pkg.package.name;
	// 	var appObject = { 
	// 		[appName]: {
	// 			"location": client,
	// 		}
	// 	};
	// 	interfacesArray.push(appObject);
    // });
	
	
	var appName = "Athena Interface";
	var appObject = { 
		[appName]: {
			"location": client,
		}
	};
	interfacesArray.push(appObject);
  }
  return interfacesArray;
}

async function getDirectories (src) {
	var interfacesToReturn = [];
	
	let getDirectoriesPromise = new Promise((res, rej) => {
		var res_p = res;
		var rej_p = rej;
		// glob(src + '/*/launcher_settings/interface_package.json', function(err, folders) {
		// 	console.log(folders);
		// 	if(folders) {
		// 		folders.forEach(function (folder) {
		// 			var folderToReturn = folder.replace("launcher_settings/interface_package.json", "");
		// 			interfacesToReturn.push(folderToReturn);
		// 		});
		// 		res_p();
		// 	} else {
		// 		rej_p("Failed to load directories.");
		// 	}
		// });
		
		glob(src + '/*/interface.exe', function(err, folders) {
			console.log(folders);
			if(folders) {
				folders.forEach(function (folder) {
					var folderToReturn = folder.replace("interface.exe", "");
					interfacesToReturn.push(folderToReturn);
				});
				res_p();
			} else {
				rej_p("Failed to load directories.");
			}
		});
	});
	
	let result = await getDirectoriesPromise; 
	return interfacesToReturn;
}

async function getLibraryInterfaces() {
	var interfaces = [];

	let getLibraryPromise = new Promise((res, rej) => {
		var res_p = res;
		var rej_p = rej;
		getSetting('athena_interface.library', storagePath.default).then(async function(libraryPath){
			if(libraryPath) {
				await getDirectories(libraryPath).then(function(interfacesList){
					interfaces = interfacesList;
					res_p();
				});
			} else {
				interfaces = ["Select a library folder."];
				rej_p("Select a library folder.");
			}

		});
	});

	let result = await getLibraryPromise; 
	return interfaces;
}

function setLibrary(libPath) {
	storage.set('athena_interface.library', libPath, {dataPath: storagePath.default}, function(error) {
		if (error) {
			throw error;
		} else {
			win.webContents.send('current-library-folder', {
				libPath
			});
			storagePath.currentLibrary = libPath;
			return true;
		}
	});
}

function setLibraryDialog() {
	const {dialog} = require('electron') 

	dialog.showOpenDialog(win, {
		title: "Select A Folder",
		properties: ['openDirectory'],
	}).then(result => {
		console.log("Cancelled set library dialog: " + result.canceled)
		console.log("Selected library: " + result.filePaths)
		if(!result.canceled && result.filePaths[0]) {
			setLibrary(result.filePaths[0]);
		} else {
			return false;
		}
	}).catch(err => {
		console.log(err)
		return false;
	})
}

async function getLatestVersionJSON() {
	var metaURL = 'https://projectathena.io/cdn/athena/launcher/athenaMeta.json';
		
	await electronDl.download(win, metaURL, {
		directory: storagePath.default,
		showBadge: false,
		filename: "athenaMeta.json",
		onProgress: currentProgress => {
			var percent = currentProgress.percent;
			// console.info("DLing meta:", percent);
		},
	});
	
	var athenaMetaFile = storagePath.default + '/athenaMeta.json';
	let rawdata = fs.readFileSync(athenaMetaFile);
	let athenaMetaJSON = JSON.parse(rawdata);
	
	if (athenaMetaJSON.latest) {
		console.info("Athena Meta JSON:", athenaMetaJSON);
		return athenaMetaJSON.latest;
	} else {
		return false;
	}
}

async function getSetting(setting, storageDataPath) {
	var returnValue;

	let storagePromise = new Promise((res, rej) => {
		storage.get(setting, {dataPath: storageDataPath}, function(error, data) {
			if (error) {
				returnValue = false;
				rej("Error: " + error);
				throw error;
			} else if (Object.entries(data).length==0) {
				// console.info("Requested:", setting, "Got data:", data, "Object.entries:", Object.entries(data).length);
				returnValue = false;
				rej("Not found.")
			} else {
				returnValue = data;
				res("Success!");
			}
		});
	}).catch(err => {
		console.info("Attempted to retrieve:", setting, "from:", storageDataPath, "but got:", err)
	})

	// because async won't work otherwise. 
	let result = await storagePromise; 
	console.info("getSetting Return Value:",returnValue)
	return returnValue;
}

const { ipcMain } = require('electron')

ipcMain.on('save-state', (event, arg) => {
	storage.set('athena_launcher.state', arg, {dataPath: storagePath.default}, function(error) {
		if (error) throw error;
	});
})

ipcMain.on('load-state', (event, arg) => {
	getSetting('athena_launcher.state', storagePath.default).then(function(results){
		if(results) {
			win.webContents.send('state-loaded', {
				results
			});
		}
	});
})

ipcMain.on('set-metaverse-server', (event, arg) => {
	if(arg != "") {
		process.env.HIFI_METAVERSE_URL = arg;
	} else {
		delete process.env.HIFI_METAVERSE_URL;
	}
	console.info("Current env:", process.env.HIFI_METAVERSE_URL)
})

ipcMain.on('launch-interface', (event, arg) => {
	var interface_exe = require('child_process').execFile;
	// var executablePath = "E:\\Development\\High_Fidelity\\v0860-kasen-VS-release+freshstart\\build\\interface\\Packaged_Release\\Release\\interface.exe";
	var executablePath = arg.exec;
	var parameters = [];

	// arg is expected to be true or false with regards to SteamVR being enabled or not, later on it may be an object or array and we will handle it accordingly.
	if (arg.steamVR) {
		parameters.push('--disable-displays="OpenVR (Vive)"');
		parameters.push('--disable-inputs="OpenVR (Vive)"');
	}
	
	if (arg.allowMultipleInstances) {
		parameters.push('--allowMultipleInstances');
	}
		
	console.info("Nani?", parameters, "type?", Array.isArray(parameters));

	// interface_exe(executablePath, parameters, { windowsVerbatimArguments: true }, function(err, stdout, data) {
	// 	console.log(err)
	// 	console.log(stdout.toString());
	// });
  
})

ipcMain.on('get-athena-location', async (event, arg) => {
	var athenaLocation = await getSetting('athena_interface.location', storagePath.interfaceSettings);
	var athenaLocationExe = athenaLocation.toString();
	console.info("AthenaLocationExe:",athenaLocationExe);
	event.returnValue = athenaLocationExe;
})

ipcMain.on('set-athena-location', async (event, arg) => {
  const {dialog} = require('electron') 
  
  dialog.showOpenDialog(win, {
    title: "Select the Athena Interface executable",
    properties: ['openFile'],
    defaultPath: storage.getDataPath(),
    filters: [
      { name: 'Interface Executable', extensions: ['exe'] },
    ]
  }).then(result => {
    console.log(result.canceled)
    console.log(result.filePaths)
    if(!result.canceled && result.filePaths[0]) {
		var storageSavePath;
		if (storagePath.interfaceSettings) {
			storageSavePath = storagePath.interfaceSettings;
		} else {
			storageSavePath = storagePath.defaultPath;
		}
		storage.set('athena_interface.location', result.filePaths[0], {dataPath: storageSavePath}, function(error) {
			if (error) throw error;
		});
    } else {
      return;
    }
  }).catch(err => {
    console.log(err)
    return;
  })
  
})

ipcMain.on('setLibraryFolder', (event, arg) => {
	setLibraryDialog();
})

ipcMain.on('getLibraryFolder', (event, arg) => {
	getSetting('athena_interface.library', storagePath.default).then(async function(libraryPath){
		win.webContents.send('current-library-folder', {
			libraryPath
		});
		storagePath.currentLibrary = libraryPath;
	});
})

ipcMain.on('setCurrentInterface', (event, arg) => {
	if(arg) {
		storage.setDataPath(arg + "/launcher_settings");
		storagePath.interface = arg;
		storagePath.interfaceSettings = arg + "/launcher_settings";
		console.info("storagePath:", JSON.stringify(storagePath));
	}
})

ipcMain.handle('isInterfaceSelectionRequired', (event, arg) => {
	if(storagePath.interface == null || storagePath.interfaceSettings == null) {
		event.sender.send('interface-selection-required', true);
	} else {
		event.sender.send('interface-selection-required', false);
	}
})

ipcMain.handle('populateInterfaceList', (event, arg) => {
	getLibraryInterfaces().then(async function(results) {
		var generatedList = await generateInterfaceList(results);
		// console.info("Returning...", generatedList, "typeof", typeof generatedList, "results", results);
		event.sender.send('interface-list', generatedList);
	});
})

ipcMain.handle('get-interface-list-for-launch', (event, arg) => {
	getLibraryInterfaces().then(async function(results) {
		var generatedList = await generateInterfaceList(results);
		// console.info("Returning...", generatedList, "typeof", typeof generatedList, "results", results);
		event.sender.send('interface-list-for-launch', generatedList);
	});
})


ipcMain.on('download-athena', async (event, arg) => {
	var libraryPath;
	// var downloadURL = "https://files.yande.re/sample/a7e8adac62ee05c905056fcfb235f951/yande.re%20572549%20sample%20bikini%20breast_hold%20cleavage%20jahy%20jahy-sama_wa_kujikenai%21%20konbu_wakame%20swimsuits.jpg";
	var downloadURL = await getLatestVersionJSON();
	if (downloadURL) {
		getSetting('athena_interface.library', storagePath.default).then(function(results){
			if(results) {
				libraryPath = results;
				electronDl.download(win, downloadURL, {
					directory: libraryPath,
					showBadge: true,
					filename: "Athena_Setup_Latest.exe",
					onProgress: currentProgress => {
						console.info(currentProgress);
						var percent = currentProgress.percent;
						win.webContents.send('download-installer-progress', {
							percent
						});
					},
				});
			} else {
				setLibraryDialog();
			}
		});
	} else {
		console.info("Failed to retrieve download URL.");
	}
})

ipcMain.on('install-athena', (event, arg) => {
	getSetting('athena_interface.library', storagePath.default).then(function(libPath){
		var installer_exe = require('child_process').execFile;
		var executablePath = libPath + "/Athena_Setup_Latest.exe";
		var installPath = libPath + "/Athena_Interface_Latest";
		var parameters = [""];
		
		if (!fs.existsSync(executablePath)) {
			// Notify main window of the issue.
			win.webContents.send('no-installer-found');
			return;
		}
		
		console.info("Installing, params:", executablePath, installPath, parameters)

		installer_exe(executablePath, parameters, function(err, data) {
			console.log(err)
			console.log(data.toString());
		});
	}).catch(function(caught) {
		
	});
});

