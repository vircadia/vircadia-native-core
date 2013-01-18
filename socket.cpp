#include <iostream>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>

const int UDP_PORT = 55443; 
const int MAX_PACKET_SIZE = 1024;

const float SAMPLE_RATE = 22050.0;
const int SAMPLES_PER_PACKET = 512;

const int MAX_AGENTS = 1000;
const int LOGOFF_CHECK_INTERVAL = 1000;

const float BUFFER_SEND_INTERVAL = (SAMPLES_PER_PACKET/SAMPLE_RATE) * 1000;

pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

int16_t* packet_buffer;

sockaddr_in address, dest_address;
socklen_t destLength = sizeof(dest_address);

struct AgentList {
    sockaddr_in agent_addr;
    bool active;
    timeval time;
} agents[MAX_AGENTS];

int num_agents = 0;

double diffclock(timeval clock1,timeval clock2)
{
    double diffms = (clock2.tv_sec - clock1.tv_sec) * 1000.0;
    diffms += (clock2.tv_usec - clock1.tv_usec) / 1000.0;   // us to ms
    return diffms;
}

int create_socket()
{
    //  Create socket 
    int handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if (handle <= 0) {
        printf("Failed to create socket: %d\n", handle);
        return false;
    }

    return handle;
}

int network_init()
{
    int handle = create_socket();
    
    //  Bind socket to port 
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( (unsigned short) UDP_PORT );
    
    if (bind(handle, (const sockaddr*) &address, sizeof(sockaddr_in)) < 0) {
        printf( "failed to bind socket\n" );
        return false;
    }   
        
    return handle;
}

int addAgent(sockaddr_in dest_address) {
    //  Search for agent in list and add if needed 
    int is_new = 0; 
    int i = 0;

    for (i = 0; i < num_agents; i++) {
        if (dest_address.sin_addr.s_addr == agents[i].agent_addr.sin_addr.s_addr
            && dest_address.sin_port == agents[i].agent_addr.sin_port) {
            break;
        }        
    }
    
    if ((i == num_agents) || (agents[i].active == false)) {
        is_new = 1;
    }

    agents[i].agent_addr = dest_address; 
    agents[i].active = true;
    gettimeofday(&agents[i].time, NULL);
    
    if (i == num_agents) {
        num_agents++;
    }

    return is_new;
}

void update_agent_list(timeval now) {
    int i;
    // std::cout << "Checking agent list" << "\n";
    for (i = 0; i < num_agents; i++) {
        if ((diffclock(agents[i].time, now) > LOGOFF_CHECK_INTERVAL) &&
                agents[i].active) {
            std::cout << "Expired Agent #" << i << "\n";
            agents[i].active = false; 
        }
    }
}

struct send_buffer_struct {
    int socket_handle;
};

void *send_buffer_thread(void *args)
{
    struct send_buffer_struct *buffer_args = (struct send_buffer_struct *) args;
    int handle = buffer_args->socket_handle;

    int sent_bytes;
    timeval last_send, now;

    gettimeofday(&last_send, NULL);
    gettimeofday(&now, NULL);

    while (true) {
        while (diffclock(last_send, now) < BUFFER_SEND_INTERVAL) {
            // loop here until we're allowed to send the buffer
            gettimeofday(&now, NULL);
        }
        
        // send out whatever we have in the buffer as mixed audio
        // to our recent clients

        for (int i = 0; i < num_agents; i++) {
            if (agents[i].active) {
                sockaddr_in dest_address = agents[i].agent_addr;
                
                pthread_mutex_lock(&buffer_mutex);
                
                if (packet_buffer != NULL) {
                    sent_bytes = sendto(handle, packet_buffer, MAX_PACKET_SIZE,
                                0, (sockaddr *) &dest_address, sizeof(dest_address));
                }
                
                pthread_mutex_unlock(&buffer_mutex);
                
                if (sent_bytes < MAX_PACKET_SIZE) {
                    std::cout << "Error sending mix packet! " << sent_bytes << strerror(errno) << "\n";
                }
            }
        }

        packet_buffer = NULL;
        gettimeofday(&last_send, NULL);
    }  

    pthread_exit(0);  
}

struct process_arg_struct {
    int16_t *packet_data;
    sockaddr_in dest_address;
};

void *process_client_packet(void *args)
{
    struct process_arg_struct *process_args = (struct process_arg_struct *) args;
    
    sockaddr_in dest_address = process_args->dest_address;
    
    pthread_mutex_lock(&buffer_mutex);
    
    if (packet_buffer == NULL) {
        packet_buffer = process_args->packet_data;
    } else {
        for (int i = 0; i < MAX_PACKET_SIZE; i++) {
            packet_buffer[i] = (process_args->packet_data[i] + packet_buffer[i]) / 2;
        }
    }

    pthread_mutex_unlock(&buffer_mutex);

    if (addAgent(dest_address)) {
        std::cout << "Added agent: " << 
            inet_ntoa(dest_address.sin_addr) << " on " <<
            dest_address.sin_port << "\n";
    }    

    pthread_exit(0);
}

bool different_clients(sockaddr_in addr1, sockaddr_in addr2) 
{
    return addr1.sin_addr.s_addr != addr2.sin_addr.s_addr ||
            (addr1.sin_addr.s_addr == addr2.sin_addr.s_addr &&
            addr1.sin_port != addr2.sin_port);
}

int main(int argc, const char * argv[])
{
    timeval now, last_agent_update;
    int received_bytes = 0;

    packet_buffer = NULL;
    
    int handle = network_init();
    
    if (!handle) {
        std::cout << "Failed to create listening socket.\n";
        return 0;
    } else {
        std::cout << "Network Started.  Waiting for packets.\n";
    }

    //  Set socket as non-blocking
    int nonBlocking = 1;
    if ( fcntl( handle, F_SETFL, O_NONBLOCK, nonBlocking ) == -1 )
    {
        printf( "failed to set non-blocking socket\n" );
        return false;
    }

    gettimeofday(&last_agent_update, NULL);

    int16_t packet_data[MAX_PACKET_SIZE];

    struct send_buffer_struct send_buffer_args;
    send_buffer_args.socket_handle = handle;

    pthread_t buffer_send_thread;
    pthread_create(&buffer_send_thread, NULL, send_buffer_thread, (void *)&send_buffer_args);

    while (1) {
        received_bytes = recvfrom(handle, (int16_t*)packet_data, MAX_PACKET_SIZE,
                                      0, (sockaddr*)&dest_address, &destLength);    

        if (received_bytes > 0) {
            struct process_arg_struct args;
            args.packet_data = packet_data;
            args.dest_address = dest_address;

            pthread_t client_process_thread;
            pthread_create(&client_process_thread, NULL, process_client_packet, (void *)&args);
            pthread_join(client_process_thread, NULL); 
        }

        gettimeofday(&now, NULL);
    
        if (diffclock(last_agent_update, now) > LOGOFF_CHECK_INTERVAL) {
            gettimeofday(&last_agent_update, NULL);
            update_agent_list(last_agent_update);
        } 
    }

    pthread_join(buffer_send_thread, NULL);
    
    return 0;
}

