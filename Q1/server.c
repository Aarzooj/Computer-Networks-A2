#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>

#define MAX_PROCESSES  1000

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

    // Read the required fields from /proc/[pid]/stat
    fscanf(stat_file, "%*d (%[^)]) %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %llu %llu",
       name, user_time, kernel_time);

    if (*user_time == 0 || *kernel_time == 0){
        // printf("Warning: user_time or kernel_time for process %d is zero\n", pid);
        return 0;
    }

    return 1;  // Success
}

int compare_processes_by_total_time(const void* a, const void* b){
    Process* A = (Process*) a;
    Process* B = (Process*) b;
    return (A->total_time - B->total_time); // descending order sort
}

char* top_two_CPU_processes() {
    DIR *dir = opendir("/proc");
    if (dir == NULL) {
        printf("The /proc directory doesn't exist\n");
        return NULL;
    }

    Process process_list[MAX_PROCESSES];
    int count = 0;
    struct dirent *row;

    // Iterate over each directory in /proc to find process directories (numeric PIDs)
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

    snprintf(result + strlen(result), sizeof(result) - strlen(result),
            "Process 1: PID = %d, Process Name = %s, User Time = %lld ticks, Kernel Time = %lld ticks, Total Time = %lld ticks\n",
            process_list[0].pid, process_list[0].name, process_list[0].user_time, process_list[0].kernel_time, process_list[0].total_time);

    snprintf(result + strlen(result), sizeof(result) - strlen(result),
            "Process 2: PID = %d, Process Name = %s, User Time = %lld ticks, Kernel Time = %lld ticks, Total Time = %lld ticks\n",
         process_list[1].pid, process_list[1].name, process_list[1].user_time, process_list[1].kernel_time, process_list[1].total_time);


    return result;
}

void *serve_client(void* client_socket){
    int new_socket = *(int*) client_socket;  // got client socket in dereferenced state
    free(client_socket);

    char buffer[1024]; // reads the client requests
    read(new_socket, buffer, sizeof(buffer));

    char *cur_cpu_proc = top_two_CPU_processes();

    send(new_socket, cur_cpu_proc, strlen(cur_cpu_proc), 0);

    // free(cur_cpu_proc);
    close(new_socket);
    pthread_exit(NULL);

}
int main(){

    int server_fd; // file descriptor of server socket

    // create a TCP connection
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1){
        perror("TCP connection not established");
        exit(0);
    }

    //bind the server socket to a ip address and a port number
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;  // ip address belongs to IPV4 family
    server_address.sin_port = htons(8080);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // bind the socket
    if (bind(server_fd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0){
        perror("Binding of the server socket is unsuccessful");
        exit(0);
    }

    //server is set to listen for the client requests
    if (listen(server_fd, 1000) < 0){
        perror("Listening of the server socket is failed");
        exit(0);
    }

    printf("Reached till here \n");

    // server listens to allc lient, untill the code is dirupted
    int* client_socket;
    struct sockaddr_in client_address;
    socklen_t addrlen = sizeof(client_address);
    pthread_t thread_id;



    while (1){
        client_socket = malloc(sizeof(int));
        if ((*client_socket = accept(server_fd, (struct sockaddr *) &client_address, &addrlen)) < 0){
            perror("Server cant accept client request");
            free(client_socket);
            continue;
        }

        // prints the client address the request is accepted from
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        // create a new thread to handle the client
        if (pthread_create(&thread_id ,NULL ,serve_client, (void*)client_socket) != 0){
            perror("Multithreading failed. Server not able to serve the client");
            continue;  // look for other client requests
        }

        pthread_detach(thread_id);

    }

    close(server_fd);

}