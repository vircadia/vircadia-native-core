// edited 23/11/2022 by Ujean

// import Log from "@Modules/debugging/log";
import { doAPIGet, findErrorMsg } from "src/modules/utilities/apiHelpers";
import { GetAllAssigmentsResp, GetAssigmentsResp } from "./interfaces/assignments";

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
        } catch (error) {
            const errorMessage = findErrorMsg(error);
            console.log(`Exception while attempting to get assignments: ${errorMessage}`);
            // Log.error(Log.types.API, `Exception while attempting to get assignments: ${errr}`);
        }

        return response;
    }
};
