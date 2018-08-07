High Fidelity Wireshark Plugins
---------------------------------

Install wireshark 2.4.6 or higher.

Copy these lua files into c:\Users\username\AppData\Roaming\Wireshark\Plugins

After a capture any detected High Fidelity Packets should be easily identifiable by one of the following protocols

* HF-AUDIO - Streaming audio packets
* HF-AVATAR - Streaming avatar mixer packets
* HF-ENTITY - Entity server traffic
* HF-DOMAIN - Domain server traffic
* HFUDT - All other UDP traffic
