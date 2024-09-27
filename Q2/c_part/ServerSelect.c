#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>
#include <dirent.h>
/*Reference : https://github.com/AkankshaSingal8/socket_programming/tree/main */

#define PORT 1517
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define MAX_PROCESSES  1000

/*  Structure to hold the details of a process */
typedef struct{
    int pid;
    char name[256];
    unsigned long long user_time;
    unsigned long long kernel_time;
    unsigned long long total_time;
} Process;

int get_time_for_process(int pid, char *name, unsigned long long *user_time, unsigned long long *kernel_time) {
    char stat_file_path[64];
    snprintf(stat_file_path, sizeof(stat_file_path), "/proc/%d/stat", pid);

    FILE *stat_file = fopen(stat_file_path, "r");
    if (stat_file == NULL) {
        // printf("Stat file for process with pid %d is erased or finished executing\n", pid);
        return 0;
    }

    /* Reading the required fields from /proc/[pid]/stat file */
    fscanf(stat_file, "%*d (%[^)]) %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %llu %llu",
       name, user_time, kernel_time);

    if (*user_time == 0 || *kernel_time == 0){
        // printf("Warning: user_time or kernel_time for process %d is zero\n", pid);
        return 0;
    }

    return 1;  
}

/*  Function to sort the processes in  descending order of their total time */
int compare_processes_by_total_time(const void* a, const void* b){
    Process* A = (Process*) a;
    Process* B = (Process*) b;
    return (A->total_time - B->total_time); // descending order sort
}

/*  Function to get the top two CPU processes */
char* top_two_CPU_processes() {
    DIR *dir = opendir("/proc");
    if (dir == NULL) {
        printf("The /proc directory doesn't exist\n");
        return NULL;
    }

    Process process_list[MAX_PROCESSES];
    int count = 0;
    struct dirent *row;

    /* Iterating over each directory in /proc to find process directories (numeric PIDs) */
    while ((row = readdir(dir)) != NULL && count < MAX_PROCESSES) {
        if (atoi(row->d_name) > 0) {  // Valid PID directories
            int pid = atoi(row->d_name);
            char process_name[256];
            unsigned long long user_time, kernel_time, total_time;

            // Get the user and kernel time for the process
            if (get_time_for_process(pid, process_name, &user_time, &kernel_time)) {
                total_time = user_time + kernel_time;

                // Store the process details
                Process p;
                p.pid = pid;
                strcpy(p.name, process_name);
                p.user_time = user_time;
                p.kernel_time = kernel_time;
                p.total_time = total_time;

                process_list[count++] = p;
            }
        }
    }
    closedir(dir);

    qsort(process_list, count, sizeof(Process), compare_processes_by_total_time);

    static char result[2048];
    result[0] = '\0';

    /* Printing the details of the top 2 CPU Processes */
    snprintf(result + strlen(result), sizeof(result) - strlen(result),
            "Process 1: PID = %d, Process Name = %s, User Time = %lld ticks, Kernel Time = %lld ticks, Total Time = %lld ticks\n",
            process_list[0].pid, process_list[0].name, process_list[0].user_time, process_list[0].kernel_time, process_list[0].total_time);

    snprintf(result + strlen(result), sizeof(result) - strlen(result),
            "Process 2: PID = %d, Process Name = %s, User Time = %lld ticks, Kernel Time = %lld ticks, Total Time = %lld ticks\n",
         process_list[1].pid, process_list[1].name, process_list[1].user_time, process_list[1].kernel_time, process_list[1].total_time);


    return result;
}

int main() {
    int server_fd, new_socket, client_sockets[MAX_CLIENTS], max_sd, sd;
    struct sockaddr_in server_address, client_address;
    fd_set readfds;
    char buffer[BUFFER_SIZE];

    /* Initializing client sockets array */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }

    /* Creating server socket */
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    /* Setting socket options */ 
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    /* bind the server socket to a ip address and a port number */
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    /* Listening for incoming connections */ 
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    // printf("Listening on port %d\n", PORT);
    printf("Reached till here \n");

    int addrlen = sizeof(client_address);

    while (1) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add the server socket to the set
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        // Add client sockets to set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        // Wait for activity on one of the sockets
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            perror("Select error");
        }

        // If there's activity on the server socket, it's a new connection
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr*)&client_address, (socklen_t*)&addrlen)) < 0) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            printf("New connection from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

            // Add new socket to the client_sockets array
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    printf("Adding to list of sockets as %d\n", i);
                    break;
                }
            }
        }

        // Handle IO for each client socket
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds)) {
                // Check if it was for closing, and read the incoming message
                int valread = read(sd, buffer, BUFFER_SIZE);
                if (valread == 0) {
                    // Client disconnected
                    getpeername(sd, (struct sockaddr*)&client_address, (socklen_t*)&addrlen);
                    printf("Host disconnected, IP %s, Port %d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

                    close(sd);
                    client_sockets[i] = 0;
                } else {
                    // Null-terminate the buffer and process the request
                    buffer[valread] = '\0';
                    printf("Received from client %d: %s\n", i, buffer);

                    // Simulate processing the request and get CPU process info
                    char* response = top_two_CPU_processes();

                    // Send the response back to the client
                    send(sd, response, strlen(response), 0);
                }
            }
        }
    }

    return 0;
}
