var goto = Window.prompt("Where would you like to go? (ex. @username, #location, 1.0,500.3,100.2)");
var url = "hifi://" + goto;
print("Going to: " + url);
location = url;

// If the destination location is a user or named location the new location may not be set by the time execution reaches here
// because it requires an asynchronous lookup.  Coordinate changes should be be reflected immediately, though. (ex: hifi://0,0,0)
print("URL: " + location.href);
print("Protocol: " + location.protocol);
print("Hostname: " + location.hostname);
print("Pathname: " + location.pathname);
