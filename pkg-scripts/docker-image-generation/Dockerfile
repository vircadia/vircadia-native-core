FROM ubuntu:20.04
COPY vircadia-server_2022.12.02-20221123-4ff3625-0ubuntu1-1_amd64.deb .
RUN apt update
RUN apt install libcap2-bin -y
RUN apt install systemd -y
RUN apt -y install ./vircadia-server_2022.12.02-20221123-4ff3625-0ubuntu1-1_amd64.deb