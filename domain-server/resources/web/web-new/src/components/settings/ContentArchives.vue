<template>
    <div>
        <!--  Content Archives -->
        <q-card class="my-card q-mt-md">
            <q-card-section>
                <div class="text-h5 text-center text-weight-bold q-mb-sm">Content Archives</div>
                <q-separator />
                <q-card>
                    <!-- Automatic Content Archives section -->
                    <q-card-section>
                        <p class="q-mb-xs text-body1 text-weight-bold">Automatic Content Archives</p>
                        <div class="q-mt-xs text-caption text-grey-5">Your domain server makes regular archives of the content in your domain. In the list below, you can see and download all of your backups of domain content and content settings.<a class="text-primary" :href="'#/backup-restore'">Click here to manage automatic content archive intervals.</a></div>
                        <q-table dark class="bg-grey-9" :rows="rows" hide-pagination>
                            <template v-slot:header>
                                <q-tr class="bg-primary">
                                    <q-th class="text-left">Archive Name</q-th>
                                    <q-th class="text-left">Archive Date</q-th>
                                    <q-th class="text-left">Status</q-th>
                                    <q-th auto-width>Actions</q-th> <!-- Empty column for delete buttons -->
                                </q-tr>
                            </template>
                            <template v-slot:body>
                                <q-tr v-for="(backup) in automaticBackupList" :key="backup.id" :style="[backup.isCorrupted ? `background-color: #935757` : ``]">
                                    <q-td>{{ backup.name }}</q-td>
                                    <q-td>{{ formatDate(backup.createdAtMillis) }}</q-td>
                                    <q-td>{{ backup.isCorrupted ? "Corrupted" : "" }}</q-td>
                                    <q-td>
                                        <q-btn-group>
                                            <q-btn :disable="backup.isCorrupted" flat dense icon="restore"><q-tooltip class="">Restore Content</q-tooltip></q-btn>
                                            <q-btn flat dense icon="download_for_offline" :href="'/api/backups/download/' + backup.id" target="_blank"><q-tooltip class="">Download</q-tooltip></q-btn>
                                            <q-btn @click="showDeleteDialogue(backup.name, backup.createdAtMillis, backup.id)" flat dense icon="delete"><q-tooltip>Delete</q-tooltip></q-btn>
                                        </q-btn-group>
                                    </q-td>
                                </q-tr>
                            </template>
                        </q-table>
                    </q-card-section>
                    <!-- Manual Content Archives section -->
                    <q-card-section>
                        <p class="q-mb-xs text-body1 text-weight-bold">Manual Content Archives</p>
                        <div class="q-mt-xs text-caption text-grey-5">You can generate and download an archive of your domain content right now. You can also download, delete and restore any archive listed.</div>
                        <q-btn class="q-my-md" color="primary" @click="newArchiveDialogue = true">Generate New Archive</q-btn>
                        <q-dialog v-model="newArchiveDialogue" persistent>
                            <q-card class="bg-primary q-pa-md">
                                <q-card-section class="row items-center">
                                    <span class="text-h5 q-mb-sm text-bold text-white">Generate a Content Archive</span>
                                    <span class="text-body2">This will capture the state of all the content in your domain right now, which you can save as a backup and restore from later.</span>
                                </q-card-section>
                                <q-form @submit="generateNewArchive">
                                    <q-card-section>
                                        <q-input :rules="[value => /^[a-zA-Z0-9\-_ ]+$/g.test(value) || 'Valid characters include A-z, 0-9, \' \', \'_\', and \'-\'.\'']" v-model="newArchiveName" label="Archive Name" bg-color="white" label-color="primary" standout="bg-white text-primary" class="text-subtitle1"/>
                                    </q-card-section>
                                    <q-card-actions align="center">
                                        <q-btn @click="clearNewArchiveName" flat label="Cancel" color="white" v-close-popup />
                                        <q-btn flat label="Confirm" type="submit" color="white" />
                                    </q-card-actions>
                                </q-form>
                            </q-card>
                        </q-dialog>
                        <q-table dark class="bg-grey-9" :rows="rows" hide-pagination>
                            <template v-slot:header>
                                <q-tr class="bg-primary">
                                    <q-th class="text-left">Archive Name</q-th>
                                    <q-th class="text-left">Archive Date</q-th>
                                    <q-th class="text-left">Status</q-th>
                                    <q-th auto-width>Actions</q-th> <!-- Empty column for delete buttons -->
                                </q-tr>
                            </template>
                            <template v-slot:body>
                                <q-tr v-for="(backup) in manualBackupList" :key="backup.id" :style="[backup.isCorrupted ? `background-color: #935757` : ``]">
                                    <q-td>{{ backup.name }}</q-td>
                                    <q-td>{{ formatDate(backup.createdAtMillis) }}</q-td>
                                    <q-td>{{ backup.isCorrupted ? "Corrupted" : "" }}</q-td>
                                    <q-td>
                                        <q-btn-group>
                                            <q-btn :disable="backup.isCorrupted" flat dense icon="restore"><q-tooltip class="">Restore Content</q-tooltip></q-btn>
                                            <q-btn flat dense icon="download_for_offline" :href="'/api/backups/download/' + backup.id" target="_blank"><q-tooltip class="">Download</q-tooltip></q-btn>
                                            <q-btn @click="showDeleteDialogue(backup.name, backup.createdAtMillis, backup.id)" flat dense icon="delete"><q-tooltip>Delete</q-tooltip></q-btn>
                                        </q-btn-group>
                                    </q-td>
                                </q-tr>
                            </template>
                        </q-table>
                    </q-card-section>
                </q-card>
            </q-card-section>
            <!-- CONFIRM DELETE BACKUP DIALOGUE -->
            <q-dialog v-model="confirmDeleteDialogue.show" persistent>
                        <q-card class="bg-primary q-pa-md">
                            <q-card-section class="row items-center">
                                <p class="text-h6 text-bold text-white full-width"><q-avatar icon="mdi-alert" class="q-mr-sm" text-color="warning" size="20px" font-size="20px"/>Delete <span class="text-warning">{{confirmDeleteDialogue.backupToDelete}}</span>?</p>
                                <p class="text-body2">WARNING: This cannot be undone.</p>
                            </q-card-section>
                            <q-card-actions align="center">
                                <q-btn flat label="Cancel" @click="resetDeleteDialogue()"/>
                                <q-btn flat label="Delete" @click="deleteBackup(confirmDeleteDialogue.backupID)"/>
                            </q-card-actions>
                        </q-card>
                    </q-dialog>
        </q-card>
        <!-- *END* Content Archives *END* -->
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import moment from "moment";
import { BackupsList } from "@Modules/domain/backups";
import type{ Backup } from "@Modules/domain/interfaces/backups";

export default defineComponent({
    name: "ContentArchives",

    data () {
        return {
            rows: [{}],
            newArchiveDialogue: false,
            newArchiveName: "",
            automaticBackupList: [] as Backup[],
            manualBackupList: [] as Backup[],
            confirmDeleteDialogue: { show: false, backupToDelete: "", backupID: "" }
        };
    },
    methods: {
        async getBackups (): Promise<void> {
            this.automaticBackupList = await BackupsList.getAutomaticBackupsList();
            this.manualBackupList = await BackupsList.getManualBackupsList();
        },
        showDeleteDialogue (backupName: string, backupTime: number, backupID: string): void {
            this.confirmDeleteDialogue = { show: true, backupToDelete: `${backupName}: ${moment(backupTime).format("lll")}`, backupID: backupID };
        },
        resetDeleteDialogue (): void {
            this.confirmDeleteDialogue = { show: false, backupToDelete: "", backupID: "" };
        },
        clearNewArchiveName (): void {
            this.newArchiveName = "";
        },
        generateNewArchive () {
            this.newArchiveDialogue = false;
            BackupsList.generateNewArchive(this.newArchiveName);
            this.clearNewArchiveName();
            this.getBackups();
        },
        deleteBackup (backupID: string) {
            BackupsList.deleteBackup(backupID);
            this.getBackups();
            this.resetDeleteDialogue();
        },
        formatDate (date: number) {
            return moment(date).format("lll");
        }
    },
    beforeMount () {
        this.getBackups();
    }
});
</script>
