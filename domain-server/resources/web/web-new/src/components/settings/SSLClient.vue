<template>
    <div>
        <!-- SSL ACME Client Settings -->
        <q-card class="my-card q-ma-sm">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold q-mb-sm">SSL ACME Client</div>
                <q-separator />
                <!-- ADVANCED SETTINGS SECTION -->
                <q-expansion-item v-model="isSSLClientSettingsToggled" class="q-mt-md text-subtitle1" popup icon="settings" label="Advanced Settings">
                    <q-card>
                        <!-- enable SSL ACME Client section -->
                        <q-card-section>
                            <q-toggle v-model="isSSLClientEnabled" checked-icon="check" color="positive" label="Enable ACME Client"
                                unchecked-icon="clear" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Enables ACME client that will manage the SSL certificates.</div>
                        </q-card-section>
                        <!-- ACME Directory Endpoint section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="directoryEndpoint" label="ACME Directory Endpoint"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">URL of the certificate issuer ACME directory endpoint.</div>
                        </q-card-section>
                        <!-- enable ZeroSSL REST API section -->
                        <q-card-section>
                            <q-toggle v-model="isZeroSSLEnabled" checked-icon="check" color="positive" label="Enable ZeroSSL REST API"
                                unchecked-icon="clear" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Use ZeroSSL Rest API Instead of ACME protocol.</div>
                        </q-card-section>
                        <!-- Account Key Path section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="accountKeyPath" label="Account Key Path" placeholder="<Application Data Path>/acme_account_key.pem" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Path to private key used to communicate with certificate issuer.</div>
                        </q-card-section>
                        <!-- ZeroSSL API Key section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="zeroSslAPIKey" label="ZeroSSL API Key" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">API key to use for ZeroSSL REST API requests.</div>
                        </q-card-section>
                        <!-- External Account Binding KID section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="externalAccountBindingKID" label="External Account Binding KID" />
                        </q-card-section>
                        <!-- External Account Binding MAC section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="externalAccountBindingMAC" label="External Account Binding MAC" />
                        </q-card-section>
                        <!-- Certificate Directory section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="certificateDirectory" label="Certificate Directory" placeholder="Application Data Path"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Certificate files will be stored in this directory.</div>
                        </q-card-section>
                        <!-- Certificate File Name section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="certificateFilename" label="Certificate Filename"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Certificate will be stored with this filename in Certificate Directory.</div>
                        </q-card-section>
                        <!-- Certificate Key Filename section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="certificateKeyFilename" label="Certificate Key Filename"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Certificate private key will be stored with this filename in Certificate Directory.</div>
                        </q-card-section>
                        <!-- Certificate Authority Filename section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="certificateAuthorityFilename" label="Certificate Authority Filename" placeholder="System Default"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Trusted certificate authority list will be stored with this filename in Certificate Directory. If unspecified system default CAs will be used.</div>
                        </q-card-section>
                        <!-- Type of HTTP challenge handler section -->
                        <q-card-section>
                            <q-select standout="bg-primary text-white" color="primary" emit-value map-options v-model="challengeHandler" :options="challengeHandlerOptions" label="Type of HTTP Challenge Handler" transition-show="jump-up" transition-hide="jump-down">
                                <template v-slot:prepend>
                                    <q-icon name="https" />
                                </template>
                            </q-select>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">This settings determines how the client will attempt to complete the server's HTTP challenges. Possible Values are: server - client will attempt to host the challenges on port 80, files - client will attempt to save challenges as files in the directories associated with specified domains, manual - client will wait for a few minutes for the challenges to be completed.</div>
                        </q-card-section>
                    </q-card>
                </q-expansion-item>
                <!-- *END* ADVANCED SETTINGS SECTION *END* -->
            </q-card-section>
        </q-card>
        <!-- *END* WebRTC Settings *END* -->
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { Settings } from "@Modules/domain/settings";
import type { SettingsValues, SSLClientAcmeSaveSettings } from "@Modules/domain/interfaces/settings";

export default defineComponent({
    name: "SSLClientSettings",

    data () {
        return {
            isSSLClientSettingsToggled: false,
            values: {} as SettingsValues,
            challengeHandlerOptions: [
                {
                    label: "SERVER: client will attempt to host the challenges on port 80",
                    value: "server"
                },
                {
                    label: "FILES: client will attempt to save challenges as files in the directories associated with specified domains",
                    value: "files"
                },
                {
                    label: "MANUAL: client will wait for a few minutes for the challenges to be completed",
                    value: "manual"
                }
            ]
        };
    },
    methods: {
        async refreshSettingsValues (): Promise<void> {
            this.values = await Settings.getValues();
        },
        saveSettings (): void {
            const settingsToCommit: SSLClientAcmeSaveSettings = {
                "acme": {
                    "enable_client": this.isSSLClientEnabled,
                    "directory_endpoint": this.directoryEndpoint,
                    "zerossl_rest_api": this.isZeroSSLEnabled,
                    "account_key_path": this.accountKeyPath,
                    "zerossl_api_key": this.zeroSslAPIKey,
                    "eab_kid": this.externalAccountBindingKID,
                    "eab_mac": this.externalAccountBindingMAC,
                    "certificate_directory": this.certificateDirectory,
                    "certificate_filename": this.certificateFilename,
                    "certificate_key_filename": this.certificateKeyFilename,
                    "certificate_authority_filename": this.certificateAuthorityFilename,
                    "challenge_handler_type": this.challengeHandler
                }
            };
            Settings.automaticCommitSettings(settingsToCommit);
        }
    },
    computed: {
        isSSLClientEnabled: {
            get (): boolean {
                return this.values.acme?.enable_client ?? false;
            },
            set (newEnableSSLClient: boolean): void {
                if (typeof this.values.acme?.enable_client !== "undefined") {
                    this.values.acme.enable_client = newEnableSSLClient;
                    this.saveSettings();
                }
            }
        },
        directoryEndpoint: {
            get (): string {
                return this.values.acme?.directory_endpoint ?? "error";
            },
            set (newDirectoryEndpoint: string): void {
                if (typeof this.values.acme?.directory_endpoint !== "undefined") {
                    this.values.acme.directory_endpoint = newDirectoryEndpoint;
                    this.saveSettings();
                }
            }
        },
        isZeroSSLEnabled: {
            get (): boolean {
                return this.values.acme?.zerossl_rest_api ?? false;
            },
            set (newEnableZeroSSL: boolean): void {
                if (typeof this.values.acme?.zerossl_rest_api !== "undefined") {
                    this.values.acme.zerossl_rest_api = newEnableZeroSSL;
                    this.saveSettings();
                }
            }
        },
        accountKeyPath: {
            get (): string {
                return this.values.acme?.account_key_path ?? "error";
            },
            set (newAccountKeyPath: string): void {
                if (typeof this.values.acme?.account_key_path !== "undefined") {
                    this.values.acme.account_key_path = newAccountKeyPath;
                    this.saveSettings();
                }
            }
        },
        zeroSslAPIKey: {
            get (): string {
                return this.values.acme?.zerossl_api_key ?? "error";
            },
            set (newAPIKey: string): void {
                if (typeof this.values.acme?.zerossl_api_key !== "undefined") {
                    this.values.acme.zerossl_api_key = newAPIKey;
                    this.saveSettings();
                }
            }
        },
        externalAccountBindingKID: {
            get (): string {
                return this.values.acme?.eab_kid ?? "error";
            },
            set (newEabKid: string): void {
                if (typeof this.values.acme?.eab_kid !== "undefined") {
                    this.values.acme.eab_kid = newEabKid;
                    this.saveSettings();
                }
            }
        },
        externalAccountBindingMAC: {
            get (): string {
                return this.values.acme?.eab_mac ?? "error";
            },
            set (newEabMac: string): void {
                if (typeof this.values.acme?.eab_mac !== "undefined") {
                    this.values.acme.eab_mac = newEabMac;
                    this.saveSettings();
                }
            }
        },
        certificateDirectory: {
            get (): string {
                return this.values.acme?.certificate_directory ?? "error";
            },
            set (newCertificateDirectory: string): void {
                if (typeof this.values.acme?.certificate_directory !== "undefined") {
                    this.values.acme.certificate_directory = newCertificateDirectory;
                    this.saveSettings();
                }
            }
        },
        certificateFilename: {
            get (): string {
                return this.values.acme?.certificate_filename ?? "error";
            },
            set (newCertificateFilename: string): void {
                if (typeof this.values.acme?.certificate_filename !== "undefined") {
                    this.values.acme.certificate_filename = newCertificateFilename;
                    this.saveSettings();
                }
            }
        },
        certificateKeyFilename: {
            get (): string {
                return this.values.acme?.certificate_key_filename ?? "error";
            },
            set (newCertificateKeyFilename: string): void {
                if (typeof this.values.acme?.certificate_key_filename !== "undefined") {
                    this.values.acme.certificate_key_filename = newCertificateKeyFilename;
                    this.saveSettings();
                }
            }
        },
        certificateAuthorityFilename: {
            get (): string {
                return this.values.acme?.certificate_authority_filename ?? "error";
            },
            set (newCertificateAuthorityFilename: string): void {
                if (typeof this.values.acme?.certificate_authority_filename !== "undefined") {
                    this.values.acme.certificate_authority_filename = newCertificateAuthorityFilename;
                    this.saveSettings();
                }
            }
        },
        challengeHandler: {
            get (): string {
                return this.values.acme?.challenge_handler_type ?? "error";
            },
            set (newChallengerHandler: string): void {
                if (typeof this.values.acme?.challenge_handler_type !== "undefined") {
                    this.values.acme.challenge_handler_type = newChallengerHandler;
                    this.saveSettings();
                }
            }
        }
    },
    beforeMount () {
        this.refreshSettingsValues();
    }
});
</script>
