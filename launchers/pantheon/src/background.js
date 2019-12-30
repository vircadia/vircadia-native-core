'use strict'

import { app, protocol, BrowserWindow } from 'electron'
import {
  createProtocol,
  installVueDevtools
} from 'vue-cli-plugin-electron-builder/lib'
import path from 'path'
const isDevelopment = process.env.NODE_ENV !== 'production'
const storage = require('electron-json-storage');
const electronDl = require('electron-dl');
 
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
      // devTools: true
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
  await installVueDevtools()
} catch (e) {
  // console.error('Vue Devtools failed to install:', e.toString())
}

  }
  createWindow()
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

function setLibraryDialog() {
  const {dialog} = require('electron') 
  
  dialog.showOpenDialog(win, {
    title: "Select the Athena Interface app library folder",
    properties: ['openDirectory'],
  }).then(result => {
    console.log(result.canceled)
    console.log(result.filePaths)
    if(!result.canceled && result.filePaths[0]) {
      storage.set('athena_interface.library', result.filePaths[0], function(error) {
        if (error) {
          throw error;
        } else {
          return true;
        }
      });
    } else {
      return false;
    }
  }).catch(err => {
    console.log(err)
    return false;
  })
}

async function getSetting(setting) {
  var returnValue;
  
  let storagePromise = new Promise((res, rej) => {
    storage.get(setting, function(error, data) {
      if (error) {
        throw error;
        returnValue = false;
        rej("Error: " + error);
      } else if (data == {}) {
        returnValue = false;
        rej("Not found.")
      }
      returnValue = data.toString();
      res("Success!");
    });
  });
  
  // because async won't work otherwise. 
  let result = await storagePromise; 
  return returnValue;
}

const { ipcMain } = require('electron')

ipcMain.on('launch-interface', (event, arg) => {
  var interface_exe = require('child_process').execFile;
  // var executablePath = "E:\\Development\\High_Fidelity\\v0860-kasen-VS-release+freshstart\\build\\interface\\Packaged_Release\\Release\\interface.exe";
  var executablePath = arg.exec;
  var parameters;
  
  // arg is expected to be true or false with regards to SteamVR being enabled or not, later on it may be an object or array and we will handle it accordingly.
  if(arg.steamVR) {
    parameters = ['--disable-displays', 'OpenVR (Vive)', '--disable-inputs', 'OpenVR (Vive)'];
  } else {
    parameters = [""];
  }

  interface_exe(executablePath, parameters, function(err, data) {
    console.log(err)
    console.log(data.toString());
  });
  
})

ipcMain.on('getAthenaLocation', async (event, arg) => {
  var athenaLocation = await getSetting('athena_interface.location');
  console.log(athenaLocation);
  event.returnValue = athenaLocation;
})

ipcMain.on('setAthenaLocation', async (event, arg) => {
  const {dialog} = require('electron') 
  
  dialog.showOpenDialog(win, {
    title: "Select the Athena Interface executable",
    properties: ['openFile'],
    filters: [
      { name: 'Interface Executable', extensions: ['exe'] },
    ]
  }).then(result => {
    console.log(result.canceled)
    console.log(result.filePaths)
    if(!result.canceled && result.filePaths[0]) {
      storage.set('athena_interface.location', result.filePaths[0], function(error) {
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

ipcMain.on('installAthena', (event, arg) => {
  var libraryPath;
  var downloadURL = "https://files.yande.re/sample/a7e8adac62ee05c905056fcfb235f951/yande.re%20572549%20sample%20bikini%20breast_hold%20cleavage%20jahy%20jahy-sama_wa_kujikenai%21%20konbu_wakame%20swimsuits.jpg";
  
  getSetting('athena_interface.library').then(function(results){
    if(results) {
      libraryPath = results;
      console.log("How many times?" + libraryPath);
      electronDl.download(win, downloadURL, {
        directory: libraryPath,
        showBadge: true,
        filename: "waifu.jpg"
      });
    } else {
      setLibraryDialog();
    }
  });
})

