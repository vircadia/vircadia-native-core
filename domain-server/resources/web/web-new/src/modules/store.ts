import { ref } from "vue";
import { Settings } from "@Modules/domain/settings";

class Store {
    public isLoggedIn = ref(false);
    private static instance: Store;

    public constructor () {
        isUserConnected().then((value) => { this.isLoggedIn = ref(value); console.log(this.isLoggedIn); });
    }

    public static getInstance (): Store {
        if (!Store.instance) {
            Store.instance = new Store();
        }
        return Store.instance;
    }
}

async function isUserConnected (): Promise<boolean> {
    const values = await Settings.getValues();
    return Boolean(values.metaverse?.access_token);
}

export const StoreInstance = Store.getInstance();
