# Project README

## Running the Code

To run the code, follow these steps:

1. **Open Two Terminals:**

   - In the first terminal, run the following commands:
     ```bash
     gcc -pthread server.c -o server
     taskset -c 1 ./server
     ```

   - In the second terminal, run the following commands:
     ```bash
     gcc client.c -o client
     taskset -c 2 ./client
     ```

2. **Note:**
   - The code for retrieving the top two CPU processes is still pending. I will finish that later. For now, the basic code is complete.

3. **References**

    - https://github.com/AkankshaSingal8/socket_programming/tree/main
    - https://stackoverflow.com/questions/60237123/is-there-any-method-to-run-perf-under-wsl
