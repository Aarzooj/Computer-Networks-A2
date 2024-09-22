# Computer-Networks-A2


to run the code;
1) open two terminals, on first terminal run the commands:
   gcc -pthread server.c -o server
   taskset -c 1 ./server

   on second terminal run the commands:
   gcc client.c -o client
   taskset -c 2 ./client

2) for now the get top two cpu processes code is left, will finish that later. Basic code is done ig.

   
