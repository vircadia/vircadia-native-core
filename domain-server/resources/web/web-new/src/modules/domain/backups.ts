// Created 24/01/2023 by Ujean

import { doAPIGet, findErrorMsg } from "src/modules/utilities/apiHelpers";
import { Backup, Backups } from "./interfaces/backups";

const axios = require("axios");

export const BackupsList = {
    async getAutomaticBackupsList (): Promise<Backup[]> {
        const response: Backup[] = [];
        try {
            const apiRequestUrl = "api/backups";
            const backupsResponse = await doAPIGet(apiRequestUrl) as Backups;

            // checks if it is an automatic backup. If so, appends it to the response array
            backupsResponse.backups.forEach(currentBackup => {
                if (currentBackup.isManualBackup === false) {
                    response.push(currentBackup);
                }
            });
            return response;
        } catch (error) {
            const errorMessage = findErrorMsg(error);
            console.log(errorMessage);
        }
        return response;
    },
    async getManualBackupsList (): Promise<Backup[]> {
        const response: Backup[] = [];
        try {
            const apiRequestUrl = "api/backups";
            const backupsResponse = await doAPIGet(apiRequestUrl) as Backups;

            // checks if it is an automatic backup. If so, appends it to the response array
            backupsResponse.backups.forEach(currentBackup => {
                if (currentBackup.isManualBackup === true) {
                    response.push(currentBackup);
                }
            });
            return response;
        } catch (error) {
            const errorMessage = findErrorMsg(error);
            console.log(errorMessage);
        }
        return response;
    },
    generateNewArchive (newArchiveName: string) {
        return axios.post("/api/backups", `name=${newArchiveName}`)
            .then(() => {
                console.log("Successfully created new archive.");
                return true;
            })
            .catch((response: string) => {
                console.log(`Failed to create new archive: ${response}`);
                return false;
            });
    },
    deleteBackup (backupID: string) {
        return axios.delete(`/api/backups/${backupID}`)
            .then(() => {
                console.log("Successfully deleted archive.");
                return true;
            })
            .catch((response: string) => {
                console.log(`Failed to delete archive: ${response}`);
                return false;
            });
    }
};
