# Memoria

./kernel_sim < scripts/mem/A_fifo_5_10_15.txt
./kernel_sim < scripts/mem/B_lru_5_10_15.txt
./kernel_sim < scripts/mem/C_ws_tau20_10f.txt
python3 tools/plot_memory.py # genera figs/\*.png

# Disco

./kernel_sim < scripts/disk/A_fcfs_A.txt
./kernel_sim < scripts/disk/B_sstf_A.txt
./kernel_sim < scripts/disk/C_scan_A.txt

# CPU

./kernel_sim < scripts/cpu/A_rr_q3.txt
./kernel_sim < scripts/cpu/B_rr_q5.txt
./kernel_sim < scripts/cpu/C_sjf.txt
