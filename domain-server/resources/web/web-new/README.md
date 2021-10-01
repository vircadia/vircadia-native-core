# Vircadia Domain Dashboard (vircadia-domain-dashboard)

The Domain dashboard for Vircadia virtual worlds.

## Maintainer Instructions

If you have made changes to the dashboard, you must build it and commit those built files. It is also necessary to build it if you wish to have your changes reflected in a packaged Domain server.

## Install the Dependencies
```bash
npm install
```

### Start the Dashboard in Development Mode (hot-code reloading, error reporting, etc.)
```bash
quasar dev
```

### Lint the Files
```bash
npm run lint
```

## Build the Dashboard

This automatically places the compiled dashboard into the right directory (`/dist/spa`) to be used by the Domain server after it is packaged.

```bash
quasar build
```

### Customize the configuration
See [Configuring quasar.conf.js](https://v2.quasar.dev/quasar-cli/quasar-conf-js).
