#!/usr/bin/python3

import bcc
from bcc import BPF, utils
import multiprocessing
num_cpus = multiprocessing.cpu_count()

with open("bpf_program.c") as f:
    bpf_text = f.read()

b = BPF(text=bpf_text)

for cpu in range(0, num_cpus):
    # For CPU cycles (PERF_COUNT_HW_CPU_CYCLES)
    b["clk"].open_perf_event(
        bcc.PerfType.HARDWARE, 
        bcc.PerfHWConfig.CPU_CYCLES)

    b["inst"].open_perf_event(
        bcc.PerfType.HARDWARE, 
        bcc.PerfHWConfig.INSTRUCTIONS)

b.trace_print()