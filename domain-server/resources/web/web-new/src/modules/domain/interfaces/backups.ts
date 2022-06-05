
export interface Backup {
        availabilityProgress: number;
        createdAtMillis: number;
        id: string;
        isAvailable: boolean;
        isCorrupted: boolean;
        isManualBackup: boolean;
        name: string;
    }

export interface InstalledContent {
        "creation_time": number;
        filename: string;
        "install_time": number;
        "installed_by": string;
        name: string;
    }

export interface Status {
        isRecovering: boolean;
        recoveringBackupId: string;
        recoveryProgress: number;
    }

export interface Backups {
        backups: Backup[];
        "installed_content": InstalledContent;
        status: Status;
    }
