module.exports = {
	"transpileDependencies": [
		"vuetify"
	],
	pluginOptions: {
		electronBuilder: {
			builderOptions: {
				"nsis": {
					"artifactName": "AthenaLauncherSetup.${ext}",
					"installerIcon": "src/assets/logo_256_256.ico",
					"uninstallerIcon": "src/assets/logo_256_256.ico",
					"uninstallDisplayName": "Uninstall Athena Launcher",
				},
				"win": {
					"target": "nsis",
					"icon": "src/assets/logo_256_256.ico",
					"publisherName": "https://projectathena.io/",
					"requestedExecutionLevel": "requireAdministrator"
				},
				"linux": {
					"target": [
					"AppImage",
					"deb"
					]
				},
				// "extraFiles": [
				//   "settings.json"
				// ],
				"appId": "com.athena.pantheon",
				"productName": "Athena Launcher",
			},
		},
	},
	runtimeCompiler: true
}