import Log from "@Modules/debugging/log";
import { doAPIGet, findErrorMsg } from "src/modules/utilities/apiHelpers";

export interface IpAddress {
    ip: string;
    port: number;
}

export interface Assignment {
    id: string;
    local: IpAddress;
    pool: string;
    public: IpAddress;
    type: string;
    uptime: string;
    username: string;
    uuid: string;
    version: string;
}

export interface QueuedAssignment {
    id: string;
    type: string;
}

export interface GetAssigmentsResp {
    "fulfilled": Record<string, Assignment>,
    "queued": Record<string, QueuedAssignment>
}

export interface GetAllAssigmentsResp {
    "fulfilled": Assignment[],
    "queued": QueuedAssignment[]
}

export const Assignments = {

    async getAllAssignments (): Promise<GetAllAssigmentsResp> {
        const response: GetAllAssigmentsResp = { fulfilled: [], queued: [] };

        try {
            const apiRequestUrl = "assignments.json";
            const assignmentsResponse = await doAPIGet(apiRequestUrl) as GetAssigmentsResp;

            Object.entries(assignmentsResponse.fulfilled).forEach(
                ([key, value]) => {
                    value.id = key;
                    response.fulfilled.push(value);
                }
            );

            Object.entries(assignmentsResponse.queued).forEach(
                ([key, value]) => {
                    value.id = key;
                    response.queued.push(value);
                }
            );

            return response;
        } catch (err) {
            const errr = findErrorMsg(err);
            Log.error(Log.types.API, `Exception while attempting to get assignments: ${errr}`);
        }

        return response;
    }

};
