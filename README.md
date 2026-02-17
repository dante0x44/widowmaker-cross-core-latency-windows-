# cross-core-widowmaker (Windows Edition)

**"Mechanical Sympathy is not a choice, it's a requirement."**

Dieses Tool ist kein netter Benchmark – es ist ein schonungsloser Hardware-Truth-Serum für Windows.  
Es misst die **rohe, ungeschminkte Latenz** zwischen zwei physischen CPU-Kernen – ohne Scheduler-Tricks, ohne SMT/Hyper-Threading-Rauschen, ohne Bullshit.

Es zeigt dir exakt, wie teuer es ist, wenn dein Producer auf Core 0 sitzt und dein Consumer plötzlich auf Core 4 oder Core 8 landen muss.

## 🛠 Architecture & Principles

Drei Säulen, auf denen alles steht:

1. **Physical Core Pinning**  
   Threads werden mit `SetThreadAffinityMask` hart auf bestimmte logische Prozessoren gepinnt (z. B. Producer → Core 0, Consumer → Core 4). Kein Windows-Scheduler darf dazwischenfunken. Nur echte Inter-Processor-Interrupt (IPI) + Cache-Coherency-Kosten.

2. **False Sharing Prevention**  
   Synchronization-Flags sind mit `alignas(64)` (oder besser `alignas(128)`) auf eigenen Cache-Lines platziert. Kein Ping-Pong innerhalb derselben Line – nur der echte Interconnect wird gemessen.

3. **Hardware-Level Fencing & Ordering**  
   - `__rdtscp` + `_mm_lfence` vor und nach der Messung → verhindert Instruction Reordering  
   - `std::atomic` mit `memory_order_acq_rel` (oder strengere Semantik)  
   - `_mm_pause()` im Spin-Wait → energieeffizient und scheduler-freundlich

4. **Deterministic Warmup**  
   1000–5000 Iterationen Pre-Heat, um CPU-Frequenz, Cache-Zustand und Turbo zu stabilisieren. Keine ersten 10 Läufe, die noch im RAM rumstochern.

## 🚀 Performance Specs

- TSC-basiert (RDTSCP) → sub-Nanosekunden-Auflösung  
- 100.000+ Iterationen Standard (mehr = stabilere P99-Werte)  
- Pause-Loop mit `_mm_pause()` → vermeidet Power-Throttling im Spin  
- Ergebnis in **CPU-Cycles** (roundtrip + one-way, wenn asymmetrisch)

## 📊 How to Read the Results

Beispiel-Ausgabe (realistisch für Ryzen 7000/9000 Serie, AGESA 2025/26):
Roundtrip (Cycles): Avg: 124.50 | Min: 118.00
One-Way   (Cycles): Avg: 62.25  | Min: 59.00
One-Way   (ns est): ~14.75 ns (based on 4.0GHz clock)

NOTE: Zen 3 architecture will show significantly higher latencies if you cross CCX boundaries. 
This is where software optimization separates the elite from the average.
