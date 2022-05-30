export interface MainState {
    globalConsts: {
        API_SERVER: string,
    },
}

function state (): MainState {
    return {
        globalConsts: {
            API_SERVER: "http://localhost:40100/" // this is for testing. Full build should just be "/"
        }
    };
}

export default state;
