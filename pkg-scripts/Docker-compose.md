# Running multiple containers

## _This is the instruction of how to establish seven containers to let them connect with each other using docker-compose command in a YAML file. (one domain server + six assignment clients)_


---
## _How to start the containers

 - clone the repo or single yaml file and jump into that directory
 - run the following command in your terminal

 ```
 docker-compose up -d --scale assignment-client=6
 ```

 - after running the above command, check all running and stopped containers by entering

```
docker ps
```

 ```
 docker ps -a
 ```
  - or you can find the containers on docker desktop

 ## _Starting the containers successfully_

 - start vircadia and type localhost in explore app
 - type localhost:40100 in your web explorer
 - you should see seven (or six) nodes by clicking Nodes on top

 ## _Note: the other five assigment client containers have to be started manually either through terminal or docker desktop_