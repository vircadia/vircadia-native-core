//
//  main.cpp
//  JitterTester
//
//  Created by Philip on 8/1/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include <iostream>
#ifdef _WINDOWS
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#endif
#include <cerrno>
#include <stdio.h>

#include <MovingMinMaxAvg.h>
#include <SequenceNumberStats.h>
#include <SharedUtil.h> // for usecTimestampNow
#include <SimpleMovingAverage.h>
#include <StdDev.h>

const quint64 MSEC_TO_USEC = 1000;
const quint64 LARGE_STATS_TIME = 500; // we don't expect stats calculation to take more than this many usecs

void runSend(const char* addressOption, int port, int gap, int size, int report);
void runReceive(const char* addressOption, int port, int gap, int size, int report);

int main(int argc, const char * argv[]) {
    if (argc != 7) {
        printf("usage: jitter-tests <--send|--receive> <address> <port> <gap in usecs> <packet size> <report interval in msecs>\n");
        exit(1);
    }
    const char* typeOption = argv[1];
    const char* addressOption = argv[2];
    const char* portOption = argv[3];
    const char* gapOption = argv[4];
    const char* sizeOption = argv[5];
    const char* reportOption = argv[6];
    int port = atoi(portOption);
    int gap = atoi(gapOption);
    int size = atoi(sizeOption);
    int report = atoi(reportOption);

    std::cout << "type:" << typeOption << "\n";
    std::cout << "address:" << addressOption << "\n";
    std::cout << "port:" << port << "\n";
    std::cout << "gap:" << gap << "\n";
    std::cout << "size:" << size << "\n";

    if (strcmp(typeOption, "--send") == 0) {
        runSend(addressOption, port, gap, size, report);
    } else if (strcmp(typeOption, "--receive") == 0) {
        runReceive(addressOption, port, gap, size, report);
    }
    exit(1);
}

void runSend(const char* addressOption, int port, int gap, int size, int report) {
    std::cout << "runSend...\n";

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed with error %d\n", WSAGetLastError());
        return;
    }
#endif

    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(addressOption);
    servaddr.sin_port = htons(port);

    const int SAMPLES_FOR_SECOND = 1000000 / gap;
    std::cout << "SAMPLES_FOR_SECOND:" << SAMPLES_FOR_SECOND << "\n";
    const int INTERVALS_PER_30_SECONDS = 30;
    std::cout << "INTERVALS_PER_30_SECONDS:" << INTERVALS_PER_30_SECONDS << "\n";
    const int SAMPLES_FOR_30_SECONDS = 30 * SAMPLES_FOR_SECOND;
    std::cout << "SAMPLES_FOR_30_SECONDS:" << SAMPLES_FOR_30_SECONDS << "\n";
    const int REPORTS_FOR_30_SECONDS = 30 * MSECS_PER_SECOND / report;
    std::cout << "REPORTS_FOR_30_SECONDS:" << REPORTS_FOR_30_SECONDS << "\n";

    int intervalsPerReport = report / MSEC_TO_USEC;
    if (intervalsPerReport < 1) {
        intervalsPerReport = 1;
    }
    std::cout << "intervalsPerReport:" << intervalsPerReport << "\n";
    MovingMinMaxAvg<int> timeGaps(SAMPLES_FOR_SECOND, INTERVALS_PER_30_SECONDS);
    MovingMinMaxAvg<int> timeGapsPerReport(SAMPLES_FOR_SECOND, intervalsPerReport);

    char* outputBuffer = new char[size];
    memset(outputBuffer, 0, size);

    quint16 outgoingSequenceNumber = 0;


    StDev stDevReportInterval;
    StDev stDev30s;
    StDev stDev;
    
    SimpleMovingAverage averageNetworkTime(SAMPLES_FOR_30_SECONDS);
    SimpleMovingAverage averageStatsCalcultionTime(SAMPLES_FOR_30_SECONDS);
    float lastStatsCalculationTime = 0.0f; // we add out stats calculation time in the next calculation window
    bool hasStatsCalculationTime = false;

    quint64 last = usecTimestampNow();
    quint64 lastReport = 0;

    while (true) {

        quint64 now = usecTimestampNow();
        int actualGap = now - last;


        if (actualGap >= gap) {

            // pack seq num
            memcpy(outputBuffer, &outgoingSequenceNumber, sizeof(quint16));

            quint64 networkStart = usecTimestampNow();
            int n = sendto(sockfd, outputBuffer, size, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
            quint64 networkEnd = usecTimestampNow();
            float networkElapsed = (float)(networkEnd - networkStart);

            if (n < 0) {
                std::cout << "Send error: " << strerror(errno) << "\n";
            }
            outgoingSequenceNumber++;

            quint64 statsCalcultionStart = usecTimestampNow();

            int gapDifferece = actualGap - gap;

            timeGaps.update(gapDifferece);
            timeGapsPerReport.update(gapDifferece);
            stDev.addValue(gapDifferece);
            stDev30s.addValue(gapDifferece);
            stDevReportInterval.addValue(gapDifferece);
            last = now;
            
            // track out network time and stats calculation times
            averageNetworkTime.updateAverage(networkElapsed);
            
            // for our stats calculation time, we actually delay the updating by one sample.
            // we do this so that the calculation of the average timing for the stats calculation
            // happen inside of the calculation processing. This ensures that tracking stats on
            // stats calculation doesn't side effect the remaining running time.
            if (hasStatsCalculationTime) {
                averageStatsCalcultionTime.updateAverage(lastStatsCalculationTime);
            }

            if (now - lastReport >= (report * MSEC_TO_USEC)) {

                std::cout << "\n"
                    << "SEND gap Difference From Expected\n"
                    << "Overall:\n"
                    << "min: " << timeGaps.getMin() << " usecs, "
                    << "max: " << timeGaps.getMax() << " usecs, "
                    << "avg: " << timeGaps.getAverage() << " usecs, "
                    << "stdev: " << stDev.getStDev() << " usecs\n"
                    << "Last 30s:\n"
                    << "min: " << timeGaps.getWindowMin() << " usecs, "
                    << "max: " << timeGaps.getWindowMax() << " usecs, "
                    << "avg: " << timeGaps.getWindowAverage() << " usecs, "
                    << "stdev: " << stDev30s.getStDev() << " usecs\n"
                    << "Last report interval:\n"
                    << "min: " << timeGapsPerReport.getWindowMin() << " usecs, "
                    << "max: " << timeGapsPerReport.getWindowMax() << " usecs, "
                    << "avg: " << timeGapsPerReport.getWindowAverage() << " usecs, "
                    << "stdev: " << stDevReportInterval.getStDev() << " usecs\n"
                    << "Average Execution Times Last 30s:\n"
                    << "    network: " << averageNetworkTime.getAverage() << " usecs average\n"
                    << "      stats: " << averageStatsCalcultionTime.getAverage() << " usecs average"
                    << "\n";

                stDevReportInterval.reset();
                if (stDev30s.getSamples() > SAMPLES_FOR_30_SECONDS) {
                    stDev30s.reset();
                }

                lastReport = now;
            }

            quint64 statsCalcultionEnd = usecTimestampNow();
            lastStatsCalculationTime = (float)(statsCalcultionEnd - statsCalcultionStart);
            if (lastStatsCalculationTime > LARGE_STATS_TIME) {
                qDebug() << "WARNING -- unexpectedly large lastStatsCalculationTime=" << lastStatsCalculationTime;
            }
            hasStatsCalculationTime = true;

        }
    }
    delete[] outputBuffer;

#ifdef _WIN32
    WSACleanup();
#endif
}

void runReceive(const char* addressOption, int port, int gap, int size, int report) {
    std::cout << "runReceive...\n";

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed with error %d\n", WSAGetLastError());
        return;
    }
#endif

    int sockfd, n;
    struct sockaddr_in myaddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(port);


    const int SAMPLES_FOR_SECOND = 1000000 / gap;
    std::cout << "SAMPLES_FOR_SECOND:" << SAMPLES_FOR_SECOND << "\n";
    const int INTERVALS_PER_30_SECONDS = 30;
    std::cout << "INTERVALS_PER_30_SECONDS:" << INTERVALS_PER_30_SECONDS << "\n";
    const int SAMPLES_FOR_30_SECONDS = 30 * SAMPLES_FOR_SECOND;
    std::cout << "SAMPLES_FOR_30_SECONDS:" << SAMPLES_FOR_30_SECONDS << "\n";
    const int REPORTS_FOR_30_SECONDS = 30 * MSECS_PER_SECOND / report;
    std::cout << "REPORTS_FOR_30_SECONDS:" << REPORTS_FOR_30_SECONDS << "\n";

    int intervalsPerReport = report / MSEC_TO_USEC;
    if (intervalsPerReport < 1) {
        intervalsPerReport = 1;
    }
    std::cout << "intervalsPerReport:" << intervalsPerReport << "\n";
    MovingMinMaxAvg<int> timeGaps(SAMPLES_FOR_SECOND, INTERVALS_PER_30_SECONDS);
    MovingMinMaxAvg<int> timeGapsPerReport(SAMPLES_FOR_SECOND, intervalsPerReport);

    char* inputBuffer = new char[size];
    memset(inputBuffer, 0, size);


    SequenceNumberStats seqStats(REPORTS_FOR_30_SECONDS);

    StDev stDevReportInterval;
    StDev stDev30s;
    StDev stDev;

    SimpleMovingAverage averageNetworkTime(SAMPLES_FOR_30_SECONDS);
    SimpleMovingAverage averageStatsCalcultionTime(SAMPLES_FOR_30_SECONDS);
    float lastStatsCalculationTime = 0.0f; // we add out stats calculation time in the next calculation window
    bool hasStatsCalculationTime = false;

    if (bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        std::cout << "bind failed\n";
        return;
    }

    quint64 last = 0; // first case
    quint64 lastReport = 0;
    
    while (true) {
    
        quint64 networkStart = usecTimestampNow();
        n = recvfrom(sockfd, inputBuffer, size, 0, NULL, NULL); // we don't care about where it came from

        quint64 networkEnd = usecTimestampNow();
        float networkElapsed = (float)(networkEnd - networkStart);

        if (n < 0) {
            std::cout << "Receive error: " << strerror(errno) << "\n";
        }

        // parse seq num
        quint16 incomingSequenceNumber = *(reinterpret_cast<quint16*>(inputBuffer));
        seqStats.sequenceNumberReceived(incomingSequenceNumber);

        if (last == 0) {
            last = usecTimestampNow();
            std::cout << "first packet received\n";
        } else {

            quint64 statsCalcultionStart = usecTimestampNow();
            quint64 now = usecTimestampNow();
            int actualGap = now - last;

            int gapDifferece = actualGap - gap;
            timeGaps.update(gapDifferece);
            timeGapsPerReport.update(gapDifferece);
            stDev.addValue(gapDifferece);
            stDev30s.addValue(gapDifferece);
            stDevReportInterval.addValue(gapDifferece);
            last = now;
            
            // track out network time and stats calculation times
            averageNetworkTime.updateAverage(networkElapsed);
            
            // for our stats calculation time, we actually delay the updating by one sample.
            // we do this so that the calculation of the average timing for the stats calculation
            // happen inside of the calculation processing. This ensures that tracking stats on
            // stats calculation doesn't side effect the remaining running time.
            if (hasStatsCalculationTime) {
                averageStatsCalcultionTime.updateAverage(lastStatsCalculationTime);
            }

            if (now - lastReport >= (report * MSEC_TO_USEC)) {

                seqStats.pushStatsToHistory();

                std::cout << "RECEIVE gap Difference From Expected\n"
                    << "Overall:\n"
                    << "min: " << timeGaps.getMin() << " usecs, "
                    << "max: " << timeGaps.getMax() << " usecs, "
                    << "avg: " << timeGaps.getAverage() << " usecs, "
                    << "stdev: " << stDev.getStDev() << " usecs\n"
                    << "Last 30s:\n"
                    << "min: " << timeGaps.getWindowMin() << " usecs, "
                    << "max: " << timeGaps.getWindowMax() << " usecs, "
                    << "avg: " << timeGaps.getWindowAverage() << " usecs, "
                    << "stdev: " << stDev30s.getStDev() << " usecs\n"
                    << "Last report interval:\n"
                    << "min: " << timeGapsPerReport.getWindowMin() << " usecs, "
                    << "max: " << timeGapsPerReport.getWindowMax() << " usecs, "
                    << "avg: " << timeGapsPerReport.getWindowAverage() << " usecs, "
                    << "stdev: " << stDevReportInterval.getStDev() << " usecs\n"
                    << "Average Execution Times Last 30s:\n"
                    << "    network: " << averageNetworkTime.getAverage() << " usecs average\n"
                    << "      stats: " << averageStatsCalcultionTime.getAverage() << " usecs average"
                    << "\n";
                stDevReportInterval.reset();

                if (stDev30s.getSamples() > SAMPLES_FOR_30_SECONDS) {
                    stDev30s.reset();
                }

                PacketStreamStats packetStatsLast30s = seqStats.getStatsForHistoryWindow();
                PacketStreamStats packetStatsLastReportInterval = seqStats.getStatsForLastHistoryInterval();

                std::cout << "RECEIVE Packet Stats\n"
                    << "Overall:\n"
                    << "lost: " << seqStats.getLost() << ", "
                    << "lost %: " << seqStats.getStats().getLostRate() * 100.0f << "%\n"
                    << "Last 30s:\n"
                    << "lost: " << packetStatsLast30s._lost << ", "
                    << "lost %: " << packetStatsLast30s.getLostRate() * 100.0f << "%\n"
                    << "Last report interval:\n"
                    << "lost: " << packetStatsLastReportInterval._lost << ", "
                    << "lost %: " << packetStatsLastReportInterval.getLostRate() * 100.0f << "%\n"
                    << "\n\n";

                lastReport = now;
            }

            quint64 statsCalcultionEnd = usecTimestampNow();

            lastStatsCalculationTime = (float)(statsCalcultionEnd - statsCalcultionStart);
            if (lastStatsCalculationTime > LARGE_STATS_TIME) {
                qDebug() << "WARNING -- unexpectedly large lastStatsCalculationTime=" << lastStatsCalculationTime;
            }
            hasStatsCalculationTime = true;
        }
    }
    delete[] inputBuffer;

#ifdef _WIN32
    WSACleanup();
#endif
}
