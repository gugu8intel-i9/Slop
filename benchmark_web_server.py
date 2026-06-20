#!/usr/bin/env python3
import subprocess
import time
import socket
import os
import sys

BLUE = '\033[0;34m'
GREEN = '\033[0;32m'
RED = '\033[0;31m'
NC = '\033[0m'

def run_load_test(host, port, num_requests):
    request_payload = b"GET / HTTP/1.1\r\nHost: localhost:9090\r\nConnection: close\r\n\r\n"
    
    total_bytes_received = 0
    latencies = []
    
    print(f"Executing {num_requests:,} separate TCP connection HTTP transactions...")
    
    start_test = time.perf_counter()
    for i in range(num_requests):
        t0 = time.perf_counter()
        
        # 1. Establish connection (SYN-ACK handshake)
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((host, port))
        
        # 2. Send HTTP GET Request
        s.sendall(request_payload)
        
        # 3. Read complete response back (typical HTTP payload size)
        resp = s.recv(1024)
        t1 = time.perf_counter()
        
        total_bytes_received += len(resp)
        latencies.append(t1 - t0)
        
        # 4. Close connection (FIN-ACK)
        s.close()
        
    end_test = time.perf_counter()
    
    total_time = end_test - start_test
    avg_latency = sum(latencies) / len(latencies)
    rps = num_requests / total_time
    mbps = (total_bytes_received * 8) / (total_time * 1024 * 1024)
    
    return total_time, rps, avg_latency * 1000, mbps, total_bytes_received

def main():
    print(f"{BLUE}=== Slop High-Performance Web Server Throughput & Latency Benchmark ==={NC}")
    
    # 1. Compile web_server
    print("Compiling Slop Web Server with max optimizations (-O3)...")
    subprocess.run(["python3", "slop_boot.py", "web_server.slop", "web_server.c"], check=True)
    subprocess.run(["gcc", "-O3", "-ffast-math", "-flto", "-march=native", "-I.", "web_server.c", "-o", "web_server"], check=True)
    
    # 2. Launch server in background
    print("Launching Slop Web Server in background on port 9090...")
    server_process = subprocess.Popen(["./web_server"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    
    # Give the server a few milliseconds to bind and listen
    time.sleep(0.1)
    
    try:
        # 3. Run Benchmark (1,000 separate TCP handshakes + HTTP transactions)
        num_requests = 1000
        total_time, rps, avg_latency_ms, mbps, total_bytes = run_load_test("127.0.0.1", 9090, num_requests)
        
        # 4. Print Results
        print()
        print(f"{GREEN}=== BENCHMARK RESULTS ==={NC}")
        print(f"Total Transactions Processed : {num_requests:,}")
        print(f"Total Time Elapsed           : {total_time:.4f} seconds")
        print(f"Requests per Second (RPS)    : {rps:.2f} req/sec")
        print(f"Average Latency (RTT)        : {avg_latency_ms * 1000:.2f} microseconds (µs) / {avg_latency_ms:.4f} ms")
        print(f"Network Throughput           : {mbps:.2f} Mbps")
        print(f"Total Data Transferred       : {total_bytes / 1024:.2f} KB")
        print(f"=========================")
        print()
        
    except Exception as e:
        print(f"{RED}Benchmark Error: {e}{NC}")
    finally:
        # 5. Shut down server
        print("Terminating background server process...")
        server_process.terminate()
        server_process.wait()
        print("Done.")

if __name__ == "__main__":
    main()
