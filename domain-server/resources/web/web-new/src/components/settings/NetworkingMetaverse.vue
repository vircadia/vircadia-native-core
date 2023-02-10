<template>
    <SharedMethods :restart-server-watcher="restartServer"/>
    <div>
        <!-- METAVERSE ACCOUNT SECTION/CARD -->
        <q-card class="my-card">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold">Your Metaverse Account</div>
                <div v-if="isUserConnected" class="text-overline text-positive text-center">Metaverse Account Connected</div>
                <div v-else class="text-overline text-negative text-center">Account Not Connected</div>
                <!-- METAVERSE CONNECT/DISCONNECT BUTTON -->
                <q-card-actions align="center">
                    <q-btn v-if="!isUserConnected" @click="showConfirmConnect=true" href="https://metaverse.vircadia.com/live//user/tokens/new?for_domain_server=true" target="_blank" class="q-mb-sm" padding="0.5em 2em" push color="positive" label="Connect Metaverse Account" />
                    <q-btn v-else @click="showConfirmDisconnect=true" class="q-mb-sm" padding="0.5em 1.5em" push color="negative" label="Disconnect Account" />
                    <!-- CONFIRM CONNECT ACCOUNT POPUP (for user to enter access token) -->
                    <q-dialog v-model="showConfirmConnect" persistent>
                        <q-card class="bg-primary q-pa-md">
                            <q-card-section class="row items-center">
                                <span class="text-h5 q-mb-sm text-bold text-white">Connect Account</span>
                                <span class="text-body2">If you did not successfully create an access token click <b>CANCEL</b> below and attempt to connect your account again.</span>
                            </q-card-section>
                            <q-form @submit="onConnectAccount">
                                <q-card-section>
                                    <q-input mask="xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" :rules="[value => /[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}/g.test(value) || 'Invalid Token', ]" bg-color="white" label-color="primary" standout="bg-white text-primary" class="text-subtitle1" v-model="accessToken" label="Enter Access Token"/>
                                </q-card-section>
                                <q-card-actions align="center">
                                    <q-btn @click="onDisconnectAccount" flat label="Cancel" color="white" v-close-popup />
                                    <q-btn flat label="Confirm" type="submit" color="white" />
                                </q-card-actions>
                            </q-form>
                        </q-card>
                    </q-dialog>
                    <!-- CONFIRM DISCONNECT ACCOUNT POPUP -->
                    <q-dialog v-model="showConfirmDisconnect" persistent>
                        <q-card class="bg-primary q-pa-sm">
                            <q-card-section class="row items-center">
                                <q-icon class="q-mb-sm q-mr-sm" name="warning" color="warning" size="1.5rem"/>
                                <span class="text-h5 q-mb-sm text-bold text-warning">Are you sure?</span>
                                <span class="text-body2">This will remove your domain-server OAuth access token.<br/>
                                This could cause your domain to appear offline and no longer be reachable via any place names.</span>
                            </q-card-section>
                            <q-card-actions align="center">
                                <q-btn flat label="Cancel" color="white" v-close-popup />
                                <q-btn @click="onDisconnectAccount" flat label="Confirm" color="white" v-close-popup />
                            </q-card-actions>
                        </q-card>
                    </q-dialog>
                </q-card-actions>
                <!-- *END* METAVERSE CONNECT/DISCONNECT BUTTON *END* -->
                <q-separator />
                <!-- ADVANCED SETTINGS SECTION -->
                <q-expansion-item v-model="isMetaverseSettingsToggled" class="q-mt-md text-subtitle1" popup icon="settings" label="Advanced Settings">
                    <q-card>
                        <!-- Automatic Networking section -->
                        <q-card-section v-if="showAutomaticNetworking">
                            <q-select standout="bg-primary text-white" color="primary" emit-value map-options v-model="automaticNetworking" :options="networkingOptions" label="Automatic Networking" transition-show="jump-up" transition-hide="jump-down">
                                <template v-slot:prepend>
                                    <q-icon name="public" />
                                </template>
                            </q-select>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">This defines how other nodes in the Metaverse will be able to reach your domain-server.
                            If you don't want to deal with any network settings, use full automatic networking.</div>
                         </q-card-section>
                        <!-- access token section -->
                        <q-card-section>
                            <q-input mask="xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" :rules="[value => /[a-z0-9]{8}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{12}/g.test(value) || 'Incorrect Token! Generate one by clicking `Connect Account` above', ]" standout="bg-primary text-white" class="text-subtitle1" v-model="accessToken" label="Access Token"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">This is your OAuth access token to connect this domain-server with your Metaverse account. It can be generated by clicking the 'Connect Account' button above.<br/>
                            You can also go to the Security page of your account on your Metaverse Server and generate a token with the 'domains' scope and paste it here.</div>
                        </q-card-section>

                        <div class="row">
                            <!-- domain ID section -->
                            <q-card-section v-if="isUserConnected" class="col-12 col-md">
                                <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="domainID" label="Domain ID"/>
                                <q-btn @click="showCreateNewDomainID=true" class="q-mt-xs" color="primary" label="new domain ID" />
                                <q-btn @click="onChooseFromDomains" class="q-mt-xs q-ml-xs" color="primary" label="choose from my domains" />
                                <div class="q-ml-xs q-mt-xs text-caption text-grey-5">This is your Metaverse domain ID. If you do not want your domain to be registered in the Metaverse you can leave this blank.</div>
                            </q-card-section>
                            <!-- Create new domain ID popup -->
                            <q-dialog v-model="showCreateNewDomainID" persistent>
                                <q-card class="bg-primary q-pa-md">
                                    <q-card-section class="row items-center">
                                        <span class="text-h5 q-mb-sm text-bold text-white">Create new domain ID</span>
                                        <span class="text-body2">Enter a unique name for this machine. This will help you identify which domain ID belongs to which machine.</span>
                                    </q-card-section>
                                    <q-form @submit="onNewDomainID">
                                        <q-card-section>
                                            <q-input
                                                :rules="[val => !!val || 'A name is required']"
                                                bg-color="white" label-color="primary" standout="bg-white text-primary" class="text-subtitle1"
                                                v-model="newDomainLabel" label="Enter Name/Label" />
                                        </q-card-section>
                                        <q-card-actions align="center">
                                            <q-btn @click="newDomainLabel=''" flat label="Cancel" color="white" v-close-popup />
                                            <q-btn flat label="Create" type="submit" color="white"/>
                                        </q-card-actions>
                                    </q-form>
                                </q-card>
                            </q-dialog>
                            <!-- Choose from my domains popup -->
                            <q-dialog v-model="showChooseDomains" persistent>
                                <q-card class="bg-primary q-pa-md">
                                    <q-card-section class="row items-center">
                                        <span class="text-h5 q-mb-sm text-bold text-white">Choose from your domains</span>
                                        <span class="text-body2">Choose the Metaverse domain you want this domain-server to represent. This will set your domain ID on the settings page.</span>
                                    </q-card-section>
                                    <q-form v-if="showDomainOptions" @submit="onChooseDomain">
                                        <q-card-section>
                                            <q-select standout="bg-primary text-white" color="primary" emit-value map-options v-model="currentDomainOption" :options="domainsOptions" label="Choose Domain" transition-show="jump-up" transition-hide="jump-down">
                                                <template v-slot:prepend>
                                                    <q-icon name="domain" />
                                                </template>
                                            </q-select>
                                        </q-card-section>
                                        <q-card-actions align="center">
                                            <q-btn @click="newDomainLabel=''" flat label="Cancel" color="white" v-close-popup />
                                            <q-btn flat label="Choose Domain" type="submit" color="white"/>
                                        </q-card-actions>
                                    </q-form>
                                    <q-card-section v-if="!showDomainOptions">
                                        <p class="text-warning text-subtitle1 text-weight-bold">You do not have any domains in your Metaverse account. Click `NEW DOMAIN ID` to make one.</p>
                                        <q-card-actions align="center"><q-btn @click="newDomainLabel=''; showDomainOptions=true; showChooseDomains=false" flat label="Cancel" color="warning" v-close-popup /></q-card-actions>
                                    </q-card-section>
                                    <q-inner-loading :showing="chooseDomainLoading" label="Please wait..."/>
                                </q-card>
                            </q-dialog>
                            <!-- local UDP port section -->
                            <q-card-section class="col-12 col-md">
                                <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="localUDPPort" label="Local UDP Port"/>
                                <div class="q-ml-xs q-mt-xs text-caption text-grey-5">This is the local port your domain-server binds to for UDP connections.
                                Depending on your router, this may need to be changed to unique values for each domain-server in order to run multiple full automatic networking domain-servers in the same network.<br/>
                                You can use the value 0 to have the domain-server select a random port, which will help in preventing port collisions.</div>
                            </q-card-section>
                        </div>
                        <!-- enable packet verification section -->
                        <q-card-section>
                            <q-toggle v-model="isPacketVerificationEnabled" checked-icon="check" color="positive" label="Enable Packet Verification"
                                unchecked-icon="clear" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Enable secure checksums on communication that uses the Metaverse protocol. Increases security with possibly a small performance penalty</div>
                        </q-card-section>
                        <!-- enable metadata HTTP availability section -->
                        <q-card-section>
                            <q-toggle v-model="isHTTPMetadataEnabled" checked-icon="check" color="positive" label="Enable Metadata HTTP Availability"
                                unchecked-icon="clear" />
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">Allows your domain's metadata to be accessible on the public internet via direct HTTP connection to the domain server</div>
                        </q-card-section>
                        <!-- metadata exporter HTTP port section -->
                        <q-card-section>
                            <q-input standout="bg-primary text-white" class="text-subtitle1" v-model="metadataExporterPort" label="Metadata Exporter HTTP Port"/>
                            <div class="q-ml-xs q-mt-xs text-caption text-grey-5">This is the port where the Metaverse exporter accepts connections. It listens both on IPv4 and IPv6 and can be accessed remotely, so you should make sure to restrict access with a firewall as needed.</div>
                        </q-card-section>
                    </q-card>
                </q-expansion-item>
                <!-- *END* ADVANCED SETTINGS SECTION *END* -->
            </q-card-section>
        </q-card>
        <!-- *END* METAVERSE ACCOUNT SECTION/CARD *END* -->
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { Settings } from "@Modules/domain/settings";
import { MetaverseSaveSettings, SettingsValues } from "@/src/modules/domain/interfaces/settings";
import SharedMethods from "@Components/sharedMethods.vue";

export default defineComponent({
    name: "MetaverseSettings",
    components: {
        SharedMethods
    },
    data () {
        return {
            values: {} as SettingsValues,
            networkingOptions: [
                {
                    label: "Full: update both the IP address and port to reach my server",
                    value: "full"
                },
                {
                    label: "IP Only: update just my IP address, I will open the port manually",
                    value: "ip"
                },
                {
                    label: "None: use the network information I have entered for this domain in the Metaverse Server",
                    value: "disabled"
                }
            ],
            isMetaverseSettingsToggled: false,
            showConfirmConnect: false,
            showConfirmDisconnect: false,
            showCreateNewDomainID: false,
            showChooseDomains: false,
            showDomainOptions: true,
            chooseDomainLoading: false,
            // Metaverse Account section variables
            domainsOptions: [] as any,
            currentDomainOption: "",
            isUserConnected: false,
            newDomainLabel: "",
            restartServer: false
        };
    },
    methods: {
        onConnectAccount (): void {
            this.isUserConnected = true;
            this.showConfirmConnect = false;
        },
        onDisconnectAccount (): void {
            if (this.accessToken !== "" || this.domainID !== "") {
                // set access token and domain ID to empty strings, automatically saves
                const settingsToCommit: MetaverseSaveSettings = {
                    "metaverse": {
                        "access_token": "",
                        "id": ""
                    }
                };
                Settings.commitSettings(settingsToCommit);
                this.refreshSettingsValues();
            }
        },
        async onNewDomainID (): Promise<void> {
            this.showCreateNewDomainID = false;
            const newDomainID: string = await Settings.createNewDomainID(this.newDomainLabel);
            this.newDomainLabel = "";
            if (newDomainID) { // if new domain ID successfully created, creates a positive notification
                this.$q.notify({
                    message: "Success!",
                    caption: "click `Restart Server` to apply changes",
                    color: "positive",
                    position: "top"
                });
                this.domainID = newDomainID;
                // this.toggleRestartServer();
            } else { // if new domain ID not created, creates a negative notification
                this.$q.notify({
                    message: "There was a problem creating your new domain ID",
                    caption: "please try again",
                    color: "negative",
                    position: "center"
                });
            }
        },
        async onChooseFromDomains () {
            this.showChooseDomains = true;
            this.chooseDomainLoading = true;
            const domainsList = await Settings.getDomains();
            this.chooseDomainLoading = false;
            this.domainsOptions = [];
            if (domainsList.length > 0) {
                this.currentDomainOption = this.domainID;
                domainsList.forEach(domain => {
                    this.domainsOptions.push(
                        {
                            label: `"${domain.name}" - ${domain.domainId}`,
                            value: domain.domainId
                        }
                    );
                });
            } else {
                this.showDomainOptions = false;
            }
        },
        onChooseDomain () {
            this.showChooseDomains = false;
            this.domainID = this.currentDomainOption;
        },
        async refreshSettingsValues (): Promise<void> {
            this.values = await Settings.getValues();
            this.isUserConnected = Boolean(this.accessToken); // type cast to boolean (will evaluate false if access token is empty string)
        },
        saveSettings (): void {
            const settingsToCommit: MetaverseSaveSettings = {
                "metaverse": {
                    "access_token": this.accessToken,
                    "automatic_networking": this.automaticNetworking,
                    "enable_metadata_exporter": this.isHTTPMetadataEnabled,
                    "enable_packet_verification": this.isPacketVerificationEnabled,
                    "id": this.domainID,
                    "local_port": this.localUDPPort,
                    "metadata_exporter_port": this.metadataExporterPort
                }
            };
            Settings.automaticCommitSettings(settingsToCommit);
        },
        toggleRestartServer () {
            // whenever restart-server state changes, the watcher in sharedMethods.vue will fire
            this.restartServer = !this.restartServer;
        }
    },
    computed: {
        showAutomaticNetworking (): boolean {
            return this.isUserConnected && (this.domainID.length > 0);
        },
        automaticNetworking: {
            get (): string {
                return this.values.metaverse?.automatic_networking ?? "";
            },
            set (newNetworkingSetting: string): void {
                if (typeof this.values.metaverse?.automatic_networking !== "undefined") {
                    this.values.metaverse.automatic_networking = newNetworkingSetting;
                    this.saveSettings();
                }
            }
        },
        accessToken: {
            get (): string {
                return this.values.metaverse?.access_token ?? "";
            },
            set (newAccessToken: string): void {
                if (typeof this.values.metaverse?.access_token !== "undefined") {
                    this.values.metaverse.access_token = newAccessToken;
                    this.saveSettings();
                }
            }
        },
        domainID: {
            get (): string {
                return this.values.metaverse?.id ?? "ID not found";
            },
            set (newDomainID: string): void {
                if (typeof this.values.metaverse?.id !== "undefined") {
                    this.values.metaverse.id = newDomainID;
                    this.saveSettings();
                }
            }
        },
        localUDPPort: {
            get (): string {
                return this.values.metaverse?.local_port ?? "local UDP port not found";
            },
            set (newLocalPort: string): void {
                if (typeof this.values.metaverse?.local_port !== "undefined") {
                    this.values.metaverse.local_port = newLocalPort;
                    this.saveSettings();
                }
            }
        },
        isPacketVerificationEnabled: {
            get (): boolean {
                return this.values.metaverse?.enable_packet_verification ?? false;
            },
            set (newEnablePacketVerification: boolean): void {
                if (typeof this.values.metaverse?.enable_packet_verification !== "undefined") {
                    this.values.metaverse.enable_packet_verification = newEnablePacketVerification;
                    this.saveSettings();
                }
            }
        },
        isHTTPMetadataEnabled: {
            get (): boolean {
                return this.values.metaverse?.enable_metadata_exporter ?? false;
            },
            set (newEnableMetadataExporter: boolean): void {
                if (typeof this.values.metaverse?.enable_metadata_exporter !== "undefined") {
                    this.values.metaverse.enable_metadata_exporter = newEnableMetadataExporter;
                    this.saveSettings();
                }
            }
        },
        metadataExporterPort: {
            get (): string {
                return this.values.metaverse?.metadata_exporter_port ?? "not found";
            },
            set (newExporterPort: string): void {
                if (typeof this.values.metaverse?.metadata_exporter_port !== "undefined") {
                    this.values.metaverse.metadata_exporter_port = newExporterPort;
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
