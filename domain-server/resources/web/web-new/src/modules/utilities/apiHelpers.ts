// edited 23/11/2022 by Ujean

import axios from "axios";

import { DOMAIN_SERVER_ROOT } from "src/config";
// import Log from "@Modules/debugging/log";

/**
 * Extract the error string from a thrown error.
 *
 * A "catch" can get anything that is thrown. We want the error message for display
 * so this routine looks at the error object and, if a string, presumes that is the
 * error message and, if an object with the property "message", presumes that is
 * the error message. Otherwise a JSON.stringified version of the object is returned.
 *
 * @param pErr error object caught by "catch"
 * @returns the extracted error message string
 */
export function findErrorMsg (pErr: unknown): string {
    if (typeof pErr === "string") {
        return pErr;
    }
    const errr = <Error>pErr;
    if ("message" in errr) {
        return errr.message;
    }
    return `Error: ${JSON.stringify(pErr)}`;
}

export function buildUrl (pAPIUrl: string, pAPIBaseUrl?: string): string {
    // eslint-disable-next-line @typescript-eslint/restrict-plus-operands
    return (pAPIBaseUrl ?? DOMAIN_SERVER_ROOT) + pAPIUrl;
}

export async function doAPIGet (pAPIUrl: string, pAPIBaseUrl?: string): Promise<unknown> {
    const accessUrl = buildUrl(pAPIUrl, pAPIBaseUrl);

    let errorString = "";
    try {
        const response = await axios.get(accessUrl);
        if (response && response.status) {
            if (response.status === 200) {
                return response.data;
            }
            errorString = `${response.statusText ?? "unspecified"}`;
        } else {
            errorString = `Poorly formed response to GET ${pAPIUrl}: ${JSON.stringify(response)}`;
        }
    } catch (error) {
        const errorMessage = findErrorMsg(error);
        errorString = `Exception on GET ${pAPIUrl}: ${errorMessage}`;
        console.log(errorString);
        // Log.error(Log.types.API, `Exception on GET ${pAPIUrl}: ${errMsg}`);
    }
    throw new Error(errorString);
}

export async function doAPIDelete (pAPIUrl: string, pAPIBaseUrl?: string) {
    const accessUrl = buildUrl(pAPIUrl, pAPIBaseUrl);

    let errorString = "";
    try {
        const response = await axios.delete(accessUrl);
        if (response && response.status) {
            if (response.status === 200) {
                return;
            }
            errorString = `${response.statusText ?? "unspecified"}`;
        } else {
            errorString = `Poorly formed response to DELETE ${pAPIUrl}: ${JSON.stringify(response)}`;
        }
    } catch (error) {
        const errorMessage = findErrorMsg(error);
        errorString = `Exception on DELETE ${pAPIUrl}: ${errorMessage}`;
        console.log(errorString);
        // Log.error(Log.types.API, `Exception on DELETE ${pAPIUrl}: ${errMsg}`);
    }
    throw new Error(errorString);
}
