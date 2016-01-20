### Console

The High Fidelity Desktop Console, made with [Electron](http://electron.atom.io/).

### Running Locally

Make sure you have [Node.js](https://nodejs.org/en/) installed. Use the latest stable version.

```
npm install
npm start
```

To run, the console needs to find a build of Interface, domain-server, and assignment-client.

The command `npm start` tells the console to look for builds of those binaries in a build folder beside this console folder.

On platforms with separate build folders for release and debug libraries `npm start` will choose the debug binaries. On those platforms if you prefer to use local release builds you'll want `npm run local-release`.

### Packaging

CMake produces a target `package-console` that will bundle up everything you need for the console on your platform.
It ensures that there are available builds for the domain-server, assignment-client, and Interface. Then it produces an executable for the console.

Finally it copies all of the produced executables to a directory, ready for testing or packaging for deployment.
