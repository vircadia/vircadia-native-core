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

const int MAX_AGENTS = 1000;
const int LOGOFF_CHECK_INTERVAL = 1000;

const int UDP_PORT = 55443; 

const int BUFFER_LENGTH_BYTES = 1024;
const int BUFFER_LENGTH_SAMPLES = BUFFER_LENGTH_BYTES / sizeof(int16_t);
const float SAMPLE_RATE = 22050.0;
const int SAMPLES_PER_PACKET = 512;
const float BUFFER_SEND_INTERVAL = (SAMPLES_PER_PACKET/SAMPLE_RATE) * 1000;

const int NUM_SOURCE_BUFFERS = 10;

int16_t* wc_noise_buffer;

sockaddr_in address, dest_address;
socklen_t destLength = sizeof(dest_address);

struct AgentList {
    sockaddr_in agent_addr;
    bool active;
    timeval time;
} agents[MAX_AGENTS];

int num_agents = 0;

struct SourceBuffer {
    int16_t sourceAudioData[BUFFER_LENGTH_BYTES];
    timeval receiveTime;
    bool available;
} sourceBuffers[NUM_SOURCE_BUFFERS];

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

    int sentBytes;
    int currentSample = 1;
    timeval firstSend;
    timeval lastSend = {};
    timeval now;

    int16_t *clientMix = new int16_t[BUFFER_LENGTH_SAMPLES];

    gettimeofday(&firstSend, NULL);
    gettimeofday(&now, NULL);

    while (true) {
        while (lastSend.tv_sec != 0 && diffclock(lastSend, now) < BUFFER_SEND_INTERVAL) {
            // loop here until we're allowed to send the buffer
            gettimeofday(&now, NULL);
        }

        gettimeofday(&lastSend, NULL);
        sentBytes = 0;

        for (int i = 0; i < num_agents; i++) {
            if (agents[i].active) {
                sockaddr_in dest_address = agents[i].agent_addr;

                int sampleOffset = currentSample * BUFFER_LENGTH_SAMPLES;
                memcpy(clientMix, wc_noise_buffer + sampleOffset, BUFFER_LENGTH_BYTES);

                for (int b = 0; b < NUM_SOURCE_BUFFERS; b++) {
                    if (!sourceBuffers[b].available) {
                        int outputOffset = 0, dataSampleLength;

                        int receiveSample = floor(diffclock(firstSend, sourceBuffers[b].receiveTime) * (SAMPLE_RATE / 1000));
                        int receiveOffset = receiveSample - sampleOffset - BUFFER_LENGTH_SAMPLES;
                        
                        if (receiveOffset >= 0) {
                            outputOffset = receiveOffset;
                            dataSampleLength = BUFFER_LENGTH_SAMPLES - receiveOffset;
                        } else {
                            dataSampleLength = BUFFER_LENGTH_SAMPLES + receiveOffset;
                        }

                        std::cout << "SO: " << outputOffset << " DL: " << dataSampleLength << ". \n";

                        for (int s = outputOffset; s < dataSampleLength; s++) {
                            // we have source buffer data for this sample
                            clientMix[s] += sourceBuffers[b].sourceAudioData[s - receiveOffset];
                        }

                        if (outputOffset == 0) {
                            sourceBuffers[b].available = true;
                        }
                    }
                }

                sentBytes = sendto(handle, clientMix, BUFFER_LENGTH_BYTES,
                                    0, (sockaddr *) &dest_address, sizeof(dest_address));
                
                if (sentBytes < BUFFER_LENGTH_BYTES) {
                    std::cout << "Error sending mix packet! " << sentBytes << strerror(errno) << "\n";
                }
            }
        }

        currentSample++; 
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

    if (addAgent(dest_address)) {
        std::cout << "Added agent: " << 
            inet_ntoa(dest_address.sin_addr) << " on " <<
            dest_address.sin_port << "\n";
    }    

    for (int b = 0; b < NUM_SOURCE_BUFFERS; b++) {
        if (sourceBuffers[b].available) {
            gettimeofday(&sourceBuffers[b].receiveTime, NULL);
            memcpy(sourceBuffers[b].sourceAudioData, process_args->packet_data, BUFFER_LENGTH_BYTES);
            sourceBuffers[b].available = false;
 
            break;
        }
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
    FILE *workclubFile = fopen("wild.raw", "r");
    
    // get length of file
    std::fseek(workclubFile, 0, SEEK_END);
    int lengthInSamples = std::ftell(workclubFile) / sizeof(int16_t);
    std::rewind(workclubFile);
    
    // read that amount of samples from the file
    wc_noise_buffer = new int16_t[lengthInSamples];
    std::fread(wc_noise_buffer, sizeof(int16_t), lengthInSamples, workclubFile);
    
    // close it
    std::fclose(workclubFile);
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

    // set source audio buffers availability to true
    for (int b = 0; b < NUM_SOURCE_BUFFERS; b++) {
        sourceBuffers[b].available = true;
    } 

    gettimeofday(&last_agent_update, NULL);

    int16_t packet_data[BUFFER_LENGTH_SAMPLES];

    struct send_buffer_struct send_buffer_args;
    send_buffer_args.socket_handle = handle;

    pthread_t buffer_send_thread;
    pthread_create(&buffer_send_thread, NULL, send_buffer_thread, (void *)&send_buffer_args);

    while (1) {
        received_bytes = recvfrom(handle, (int16_t*)packet_data, BUFFER_LENGTH_BYTES,
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

