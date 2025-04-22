#include <uapi/linux/ptrace.h>

#define MAX_CPUS 128

struct perf_delta {
    u64 clk;
    u64 inst;
    u64 time;
};

/*
Perf Arrays to read counter values for open
perf events.
*/
BPF_PERF_ARRAY(clk, MAX_CPUS);
BPF_PERF_ARRAY(inst, MAX_CPUS);

// Per Cpu Data to store start values
BPF_PERCPU_ARRAY(data, u64);

#define CLOCK_ID 0
#define INSTRUCTION_ID 1
#define TIME_ID 2

TRACEPOINT_PROBE(sched, sched_switch) {
    u32 clk_k = CLOCK_ID;
    u32 inst_k = INSTRUCTION_ID;
    u32 time = TIME_ID;

    struct perf_delta delta = {};

    int cpu = bpf_get_smp_processor_id();

    /*
    perf_read may return negative values for errors.
    If cpu id is greater than BPF_PERF_ARRAY size,
    counters values will be very large negative number.
    NOTE: Use bpf_perf_event_value is recommended over
    bpf_perf_event_read or map.perf_read() due to
    issues in ABI. map.perf_read_value() need to be
    implemented in future.
    */
    u64 clk_start = clk.perf_read(cpu);
    u64 inst_start = inst.perf_read(cpu);
    u64 time_start = bpf_ktime_get_ns();

    u64* kptr = NULL;
    kptr = data.lookup(&clk_k);
    if (likely(kptr)) {
        delta.clk = clk_start - *kptr;
        data.update(&clk_k, &clk_start);
    } else {
        data.insert(&clk_k, &clk_start);
        delta.clk = -1;
    }

    kptr = data.lookup(&inst_k);
    if (likely(kptr)) {
        delta.inst = inst_start - *kptr;
        data.update(&inst_k, &inst_start);
    } else {
        data.insert(&inst_k, &inst_start);
        delta.inst = -1;
    }

    kptr = data.lookup(&time);
    if (likely(kptr)) {
        delta.time = time_start - *kptr;
        data.update(&time, &time_start);
        
    } else {
        data.insert(&time, &time_start);
        delta.time = -1;
    }

    #define SCALE_FACTOR 1000000

    bpf_trace_printk("Deltas: instr %llu (%llu) time %llu",
        inst_start, delta.inst, delta.time);

    return 0;
}