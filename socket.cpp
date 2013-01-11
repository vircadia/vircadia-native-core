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

const int UDP_PORT = 55443; 
const int MAX_PACKET_SIZE = 1024;

const float SAMPLE_RATE = 22050.0;
const int SAMPLES_PER_PACKET = 512;

const int MAX_AGENTS = 1000;
const int LOGOFF_CHECK_INTERVAL = 1000;

char packet_data[MAX_PACKET_SIZE];

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
        printf( "failed to create socket\n" );
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
        if (dest_address.sin_addr.s_addr == agents[i].agent_addr.sin_addr.s_addr) {
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

void *send_buffer_thread(void *args)
{
    // create our send socket
    int handle = create_socket();

    if (!handle) {
        std::cout << "Failed to create buffer send socket.\n";
        // return 0;
    } else {
        std::cout << "Buffer send socket created.\n";
    }

    int sent_bytes;

    while (1) {
        // sleep for the length of a packet of audio
        usleep((SAMPLES_PER_PACKET/SAMPLE_RATE) * pow(10, 6));

        // send out whatever we have in the buffer as mixed audio
        // to our recent clients

        for (int i = 0; i < num_agents; i++) {
            if (agents[i].active) {
                
                sockaddr_in dest_address = agents[i].agent_addr;
                sent_bytes = sendto(handle, packet_data, MAX_PACKET_SIZE,
                                0, (sockaddr *) &dest_address, sizeof(dest_address));
                
                if (sent_bytes < MAX_PACKET_SIZE) {
                    std::cout << "Error sending mix packet!\n";
                }
            }
        }
    }
}

struct process_arg_struct {
    char packet_data[MAX_PACKET_SIZE];
    sockaddr_in dest_address;
};

void *process_client_packet(void *args)
{
    struct process_arg_struct *process_args = (struct process_arg_struct *) args;
    
    sockaddr_in dest_address = process_args->dest_address;
    strcpy(packet_data, process_args->packet_data);

    if (addAgent(dest_address)) {
        std::cout << "Added agent: " << 
            inet_ntoa(dest_address.sin_addr) << " on " <<
            dest_address.sin_port << "\n";
    }    

    pthread_exit(0);
}

int main(int argc, const char * argv[])
{
    timeval now, last_agent_update;
    int received_bytes = 0;
    
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

    char packet_data[MAX_PACKET_SIZE];

    pthread_t buffer_send_thread;
    pthread_create(&buffer_send_thread, NULL, send_buffer_thread, NULL);

    while (1) {
        received_bytes = recvfrom(handle, (char*)packet_data, MAX_PACKET_SIZE,
                                      0, (sockaddr*)&dest_address, &destLength);
        
        if (received_bytes > 0) {
            struct process_arg_struct args;
            strcpy(args.packet_data, packet_data);
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

