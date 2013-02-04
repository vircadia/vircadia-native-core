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
#include <fstream>
#include <limits>

const int MAX_AGENTS = 1000;
const int LOGOFF_CHECK_INTERVAL = 1000;

const int UDP_PORT = 55443; 

const int BUFFER_LENGTH_BYTES = 1024;
const int BUFFER_LENGTH_SAMPLES = BUFFER_LENGTH_BYTES / sizeof(int16_t);
const float SAMPLE_RATE = 22050.0;
const float BUFFER_SEND_INTERVAL_USECS = (BUFFER_LENGTH_SAMPLES/SAMPLE_RATE) * 1000000;

const int MAX_SAMPLE_VALUE = std::numeric_limits<int16_t>::max();
const int MIN_SAMPLE_VALUE = std::numeric_limits<int16_t>::min();

const int MAX_SOURCE_BUFFERS = 10;

int16_t* whiteNoiseBuffer;
int whiteNoiseLength;

#define ECHO_DEBUG_MODE 0

sockaddr_in address, dest_address;
socklen_t destLength = sizeof(dest_address);

struct AgentList {
    sockaddr_in agent_addr;
    bool active;
    timeval time;
} agents[MAX_AGENTS];

int num_agents = 0;

struct SourceBuffer {
    int16_t sourceAudioData[BUFFER_LENGTH_SAMPLES];
    bool transmitted;
} sourceBuffers[MAX_SOURCE_BUFFERS];

double diffclock(timeval clock1, timeval clock2)
{
    double diffms = (clock2.tv_sec - clock1.tv_sec) * 1000.0;
    diffms += (clock2.tv_usec - clock1.tv_usec) / 1000.0;   // us to ms
    return diffms;
}

double usecTimestamp(timeval *time, double addedUsecs = 0) {
    return (time->tv_sec * 1000000.0) + time->tv_usec + addedUsecs;
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

int addAgent(sockaddr_in dest_address, void *audioData) {
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

    memcpy(sourceBuffers[i].sourceAudioData, audioData, BUFFER_LENGTH_BYTES);
    sourceBuffers[i].transmitted = false;
    
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

    int sentBytes;
    int currentFrame = 1;
    timeval startTime;
    timeval now;

    int16_t *clientMix = new int16_t[BUFFER_LENGTH_SAMPLES];
    int16_t *masterMix = new int16_t[BUFFER_LENGTH_SAMPLES];

    gettimeofday(&startTime, NULL);

    while (true) {
        gettimeofday(&now, NULL);

        while (usecTimestamp(&startTime, (currentFrame * BUFFER_SEND_INTERVAL_USECS)) <= usecTimestamp(&now)) {
            sentBytes = 0;

            int sampleOffset = ((currentFrame - 1) * BUFFER_LENGTH_SAMPLES) % whiteNoiseLength;

            memcpy(masterMix, whiteNoiseBuffer + sampleOffset, BUFFER_LENGTH_BYTES);

            for (int i = 0; i < num_agents; i++) {
                if (agents[i].active) {
                    memcpy(clientMix, masterMix, BUFFER_LENGTH_BYTES);

                    for (int b = 0; b < MAX_SOURCE_BUFFERS; b++) {
                        if (b != i && !sourceBuffers[b].transmitted) {
                            for (int s =  0; s < BUFFER_LENGTH_SAMPLES; s++) {
                                // we have source buffer data for this sample
                                int mixSample = clientMix[s] + sourceBuffers[b].sourceAudioData[s];
                                
                                int sampleToAdd = std::max(mixSample, MIN_SAMPLE_VALUE);
                                sampleToAdd = std::min(sampleToAdd, MAX_SAMPLE_VALUE);

                                clientMix[s] = sampleToAdd;
                            }

                            sourceBuffers[b].transmitted = true;
                        }
                    }

                    sockaddr_in dest_address = agents[i].agent_addr;

                    sentBytes = sendto(handle, clientMix, BUFFER_LENGTH_BYTES,
                                    0, (sockaddr *) &dest_address, sizeof(dest_address));
                
                    if (sentBytes < BUFFER_LENGTH_BYTES) {
                        std::cout << "Error sending mix packet! " << sentBytes << strerror(errno) << "\n";
                    }

                }
            }   

            currentFrame++;
        }
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

    if (addAgent(dest_address, process_args->packet_data)) {
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

void white_noise_buffer_init() {
    // open a pointer to the audio file
    FILE *whiteNoiseFile = fopen("wild.raw", "r");
    
    // get length of file
    std::fseek(whiteNoiseFile, 0, SEEK_END);
    whiteNoiseLength = std::ftell(whiteNoiseFile) / sizeof(int16_t);
    std::rewind(whiteNoiseFile);
    
    // read that amount of samples from the file
    whiteNoiseBuffer = new int16_t[whiteNoiseLength];
    std::fread(whiteNoiseBuffer, sizeof(int16_t), whiteNoiseLength, whiteNoiseFile);
    
    // close it
    std::fclose(whiteNoiseFile);
}

int main(int argc, const char * argv[])
{
    timeval now, last_agent_update;
    int received_bytes = 0;

    // read in the workclub white noise file as a base layer of audio
    white_noise_buffer_init();
    
    int handle = network_init();
    
    if (!handle) {
        std::cout << "Failed to create listening socket.\n";
        return 0;
    } else {
        std::cout << "Network Started.  Waiting for packets.\n";
    }

    //  Set socket as non-blocking
    int nonBlocking = 1;
    if (fcntl(handle, F_SETFL, O_NONBLOCK, nonBlocking) == -1) {
        printf( "failed to set non-blocking socket\n" );
        return false;
    }

    gettimeofday(&last_agent_update, NULL);

    int16_t packet_data[BUFFER_LENGTH_SAMPLES];

    for (int b = 0; b < MAX_SOURCE_BUFFERS; b++) {
        sourceBuffers[b].transmitted = true;
    }

    struct send_buffer_struct send_buffer_args;
    send_buffer_args.socket_handle = handle;

    pthread_t buffer_send_thread;
    pthread_create(&buffer_send_thread, NULL, send_buffer_thread, (void *)&send_buffer_args);

    while (1) {
        received_bytes = recvfrom(handle, (int16_t*)packet_data, BUFFER_LENGTH_BYTES,
                                      0, (sockaddr*)&dest_address, &destLength);    

        if (received_bytes > 0) {
            if (ECHO_DEBUG_MODE) {
                sendto(handle, packet_data, BUFFER_LENGTH_BYTES,
                            0, (sockaddr *) &dest_address, sizeof(dest_address));
            } else {
                struct process_arg_struct args;
                args.packet_data = packet_data;
                args.dest_address = dest_address;

                pthread_t client_process_thread;
                pthread_create(&client_process_thread, NULL, process_client_packet, (void *)&args);
                pthread_join(client_process_thread, NULL); 
            }
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

