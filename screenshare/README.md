# Hifi-Screenshare

The Screenshare app will allow easy desktop sharing by being launced from within the highfidelity interface.

# Setup
Create the following environment variable the hifi-screenshare app will use to get the proper connection info:
hifiScreenshareURL="<URL for authentication>"

# Screenshare API
In order to launch the hifi-screenshare app from within interface, you will call the following:
Screenshare.startScreenshare(displayName, userName, token, sessionID, apiKey);
The app won't run without the correct info.

# Files included
packager.js : 
Calling npm run packager will use this file to create the actual electron hifi-screenshare executable

src/main.js :
The main process file to configure the electron app

srce/app.js :
The render file to dispaly the screenshare UI