# Screen Sharing within High Fidelity
This Screen Share app, built using Electron, allows for easy desktop screen sharing when used in conjuction with various scripts in the `vircadia-content` repository.

# Screen Sharing Source Files
## `packager.js`
Calling npm run packager will use this file to create the actual Electron `hifi-screenshare` executable.
It will kick out a folder `hifi-screenshare-<platform>` which contains an executable.

## `src/screenshareApp.js`
The main process file to configure the electron app.

## `src/screenshareMainProcess.js`
The render file to display the app's UI.

## `screenshareApp.html`
The HTML that displays the screen selection UI and the confirmation screen UI.