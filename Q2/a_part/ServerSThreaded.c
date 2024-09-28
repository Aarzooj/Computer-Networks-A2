#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
/*Reference : https://github.com/AkankshaSingal8/socket_programming/tree/main */

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
        // printf("ERROR: Stat file for process with pid %d is erased or finished executing\n", pid);
        return 0;
    }

    /* Reading the required fields from /proc/[pid]/stat file */
    fscanf(stat_file, "%*d (%[^)]) %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %llu %llu",
       name, user_time, kernel_time);

    fclose(stat_file); // Ensure the file is closed

    if (*user_time == 0 || *kernel_time == 0){
        //printf("Warning: user_time or kernel_time for process %d is zero\n", pid);
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

            // Getting the user and kernel time for the process
            if (get_time_for_process(pid, process_name, &user_time, &kernel_time)) {
                total_time = user_time + kernel_time;

                // Storing the process details
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

    // Only sort if we have processes
    if (count > 0) {
        qsort(process_list, count, sizeof(Process), compare_processes_by_total_time);
    }

    static char result[2048];
    result[0] = '\0';

    // Safeguard against less than 2 processes
    if (count > 0) {
        snprintf(result + strlen(result), sizeof(result) - strlen(result),
                "Process 1: PID = %d, Process Name = %s, User Time = %llu ticks, Kernel Time = %llu ticks, Total Time = %llu ticks\n",
                process_list[0].pid, process_list[0].name, process_list[0].user_time, process_list[0].kernel_time, process_list[0].total_time);
    }

    if (count > 1) {
        snprintf(result + strlen(result), sizeof(result) - strlen(result),
                "Process 2: PID = %d, Process Name = %s, User Time = %llu ticks, Kernel Time = %llu ticks, Total Time = %llu ticks\n",
                process_list[1].pid, process_list[1].name, process_list[1].user_time, process_list[1].kernel_time, process_list[1].total_time);
    }

    return result;
}

void serve_client(int* client_socket){
    int new_socket = *client_socket;  // got client socket in dereferenced state
    free(client_socket);

    char buffer[1024]; // reading the client requests
    int read_response1 = read(new_socket, buffer, sizeof(buffer) - 1); // leave space for null terminator
    if (read_response1 < 0) {
        perror("Failed to read client request");
        close(new_socket);
        return;
    }

    buffer[read_response1] = '\0'; // null-terminate the received data
    printf("Client request: %s\n", buffer);

    char *cur_cpu_proc = top_two_CPU_processes();

    // Sending the response to the client
    if (send(new_socket, cur_cpu_proc, strlen(cur_cpu_proc), 0) < 0) {
        perror("Failed to send data to client");
    }

    // free(cur_cpu_proc);
    close(new_socket);

}
int main(){

    int server_fd; // file descriptor of server socket

    /* creating a TCP connection */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1){
        perror("TCP connection not established");
        exit(0);
    }

    /* bind the server socket to a ip address and a port number */
    struct sockaddr_in server_address;
    /* ip address belongs to IPV4 family */
    server_address.sin_family = AF_INET; 
    server_address.sin_port = htons(1516);
    server_address.sin_addr.s_addr = INADDR_ANY;

    /* binding the socket*/
    if (bind(server_fd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0){
        perror("Binding of the server socket is unsuccessful");
        exit(0);
    }

    /* server is set to listen for the client requests */
    if (listen(server_fd, 1000) < 0){
        perror("Listening of the server socket is failed");
        exit(0);
    }

    printf("Server activated \n");

    /* server listens to all client, untill the code is dirupted */
    int* client_socket;
    struct sockaddr_in client_address;
    socklen_t addrlen = sizeof(client_address);

    while (1){
        client_socket = malloc(sizeof(int));
        if ((*client_socket = accept(server_fd, (struct sockaddr *) &client_address, &addrlen)) < 0){
            perror("Server cant accept client request");
            free(client_socket);
            continue;
        }

        /* prints the client address the request is accepted from */
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        /* Serving the client sequentially in the main thread */
        serve_client(client_socket);

    }

    close(server_fd);
    return 0;
}