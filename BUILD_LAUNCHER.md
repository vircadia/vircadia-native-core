This is a stand-alone guide for building the Pantheon launcher for Athena interface on Windows 64-bit.  

## Building Pantheon Launcher

### Dependencies

You will need Node.js, Vue CLI, and the Vue CLI Electron Builder plugin.

#### Step 1. Node.js

If you don’t have Node.js, download and install [the latest LTS](https://nodejs.org/en/). Vue CLI requires Node.js version 8.9 or above (8.11.0+ recommended).

#### Step 2. Vue CLI 

To install Vue CLI, use one of the following commands. You need administrator privileges to execute these unless npm was installed on your system through a Node.js version manager (e.g. n or nvm).

```
npm install -g @vue/cli
# OR
yarn global add @vue/cli
```

After installation, you will have access to the vue binary in your command line. You can verify that it is properly installed by simply running vue, which should present you with a help message listing all available commands.

You can check the version with this command:

```
vue --version
```

#### Step 3. Vue CLI Plugin Electron Builder

Open a terminal in the Pantheon directory:

```
cd launchers/pantheon
```

Then, install and invoke the generator of vue-cli-plugin-electron-builder by running:

```
vue add electron-builder
```

### Test, Run, and Build

#### On first setup

We need to get the modules and dependencies installed.

If you use Yarn:
```
yarn install
```
or if you use NPM:
```
npm install
```

#### To start a development server:

You will need to run any of these commands with administrative privileges.

If you use Yarn:
```
yarn electron:serve
```
or if you use NPM:
```
npm run electron:serve
```
#### To build the launcher:

With Yarn:
```
yarn electron:build
```
or with NPM:
```
npm run electron:build
```

Building the launcher will generate an installer executable and an unpacked folder that allows you to run the built launcher without running the setup. You can find the build artifacts in the **dist_electron** folder. 

To see more documentation on the Vue CLI Plugin Electron Builder, visit this [website](https://nklayman.github.io/vue-cli-plugin-electron-builder/guide/guide.html).
