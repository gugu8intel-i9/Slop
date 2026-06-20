#include <iostream>
#include <vector>
#include <string>
#include <time.h>
#include <fstream>
#include <stdint.h>
#include <stdlib.h>

struct AVPacket {
    int64_t pts;
    int64_t dts;
    int size;
    char* data;
};

struct AVFrame {
    int width;
    int height;
    int format;
    uint8_t* data[4];
};

void print_ram_usage() {
    std::ifstream statm("/proc/self/status");
    std::string line;
    while (std::getline(statm, line)) {
        if (line.find("VmRSS:") != std::string::npos) {
            std::cout << line << std::endl;
            break;
        }
    }
}

int main() {
    std::cout << "--- [C++ FFmpeg Simulation: 10 Million Frame Allocations] ---" << std::endl;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    long long iterations = 10000000; // 10 Million video packet transfers
    long long count = 0;
    
    while (count < iterations) {
        // 1. Simulate malloc of AVPacket and raw byte buffers
        AVPacket* pkt = (AVPacket*)malloc(sizeof(AVPacket));
        pkt->pts = count;
        pkt->dts = count;
        pkt->size = 128;
        pkt->data = (char*)malloc(128); // 128-byte raw payload
        
        // 2. Simulate malloc of AVFrame
        AVFrame* frame = (AVFrame*)malloc(sizeof(AVFrame));
        frame->width = 1920;
        frame->height = 1080;
        frame->format = 0;
        frame->data[0] = (uint8_t*)malloc(1024); // Simulate decoded plane buffer
        
        // 3. Simple calculation simulating decoding/processing step
        pkt->pts = pkt->pts + 1;
        frame->format = pkt->size;
        
        // 4. Simulate manual free deallocations
        free(frame->data[0]);
        free(frame);
        free(pkt->data);
        free(pkt);
        
        count++;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("C++ FFmpeg Simulation Time: %.6f seconds\n", elapsed);

    print_ram_usage();
    return 0;
}
