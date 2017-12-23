 # this guide is specific to Ubuntu 16.04.
 # deb packages of High Fidelity domain server and assignment client are stored on debian.highfidelity.com
sudo su -
apt-get -y update
apt-get install -y software-properties-common
apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 15FF1AAE
add-apt-repository "deb http://debian.highfidelity.com stable main"
apt-get -y update
apt-get install -y hifi-domain-server
apt-get install -y hifi-assignment-client

 # When installing master/dev builds, the packages are slightly different and you just need to change the last 2 steps to:
apt-get install -y hifi-dev-domain-server
apt-get install -y hifi-dev-assignment-client

 # domain server and assignment clients should already be running. The processes are controlled via:
systemctl start hifi-domain-server
systemctl stop hifi-domain-server

 # Once the machine is setup and processes are running you should ensure that your firewall exposes port 40100 on TCP and all UDP ports. This will get your domain up and running and you could connect to it (for now) by using High Fidelity Interface and typing in the IP for the place name. (further customizations can be done via http://IPAddress:40100).
 
 # The server always depends on both hifi-domain-server and hifi-assignment-client running at the same time.
 # As an additional step, you should ensure that your packages are automatically updated when a new version goes out. You could, for example, set the automatic update checks to happen every hour (though this could potentially result in the domain being unreachable for a whole hour by new clients when they are released - adjust the update checks accordingly).
To do this you can modify /etc/crontab by adding the following lines
0 */1 * * * root apt-get update
1 */1 * * * root apt-get install --only-upgrade -y hifi-domain-server
2 */1 * * * root apt-get install --only-upgrade -y hifi-assignment-client
