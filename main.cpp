// WINDOWS VERSION - WIDOWMAKER EDIT
#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <windows.h>
#include <intrin.h>

// Konstanten
constexpr int ITERATIONS = 100000; 

// SETUP: Echte physikalische Trennung erzwingen
constexpr int CORE_PRODUCER = 0;
constexpr int CORE_CONSUMER = 4; // <--- GEÄNDERT: Zwingt den Thread auf einen entfernten Core!

struct alignas(64) Signal {
    std::atomic<uint64_t> flag = {0};
};

Signal ping;
Signal pong;

// Windows Pinning Helper
void pin_thread(int core_id) {
    HANDLE thread = GetCurrentThread();
    DWORD_PTR mask = (1ULL << core_id);
    if (SetThreadAffinityMask(thread, mask) == 0) {
        std::cerr << "CRITICAL: Pinning auf Core " << core_id << " fehlgeschlagen! (Hast du mindestens 5 Cores?)" << std::endl;
        exit(1);
    }
}

inline uint64_t get_hard_tsc() {
    unsigned int aux;
    _mm_lfence();
    uint64_t tsc = __rdtscp(&aux);
    _mm_lfence();
    return tsc;
}

void producer() {
    pin_thread(CORE_PRODUCER);
    printf("[Producer] Started on Core %d\n", CORE_PRODUCER);
    
    // Warmup
    for(int i=0; i<1000; ++i) { 
        ping.flag.store(1, std::memory_order_release);
        while (pong.flag.load(std::memory_order_acquire) != 1) _mm_pause();
        ping.flag.store(0, std::memory_order_release); 
        while (pong.flag.load(std::memory_order_acquire) == 1) _mm_pause();
    }
    printf("[Producer] Warmup done. Starting measurement...\n");

    std::vector<uint64_t> latencies;
    latencies.reserve(ITERATIONS);

    for (int i = 0; i < ITERATIONS; ++i) {
        uint64_t start = get_hard_tsc();
        
        ping.flag.store(i + 2, std::memory_order_release);
        
        while (pong.flag.load(std::memory_order_acquire) != (i + 2)) {
            _mm_pause();
        }
        
        uint64_t end = get_hard_tsc();
        latencies.push_back(end - start);
    }

    uint64_t sum = 0;
    uint64_t min_lat = -1; 
    
    for (auto val : latencies) {
        if (val < min_lat) min_lat = val;
        sum += val;
    }

    double cycles_avg = (double)sum / ITERATIONS;
    double cycles_min = (double)min_lat;

    printf("\n--- REAL CROSS-CORE RESULTS ---\n");
    printf("Roundtrip (Cycles): Avg: %.2f | Min: %.2f\n", cycles_avg, cycles_min);
    printf("One-Way   (Cycles): Avg: %.2f | Min: %.2f\n", cycles_avg / 2.0, cycles_min / 2.0);
    printf("One-Way   (ns est): ~%.2f ns (bei 4GHz assumption)\n", (cycles_min / 2.0) * 0.25);
}

void consumer() {
    pin_thread(CORE_CONSUMER);
    Sleep(100); 
    printf("[Consumer] Started on Core %d\n", CORE_CONSUMER);

    // Warmup Sync
    for(int i=0; i<1000; ++i) {
        while (ping.flag.load(std::memory_order_acquire) != 1) _mm_pause();
        pong.flag.store(1, std::memory_order_release);
        while (ping.flag.load(std::memory_order_acquire) == 1) _mm_pause();
        pong.flag.store(0, std::memory_order_release);
    }
    printf("[Consumer] Warmup done. Listening...\n");

    for (int i = 0; i < ITERATIONS; ++i) {
        while (ping.flag.load(std::memory_order_acquire) != (i + 2)) {
            _mm_pause();
        }
        pong.flag.store(i + 2, std::memory_order_release);
    }
}

int main() {
    std::thread t2(consumer);
    std::thread t1(producer);

    t1.join();
    t2.join();
    return 0;
}
