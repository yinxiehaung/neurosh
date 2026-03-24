# About
此專案為yxsh與我開發的AI函式庫(my_nn)的結合，其不同於一般的Shell，他有以下的內建指令:

| 指令| 功能 | 
|-----|---- |
|  infer   | 單次推論    |
|score|批次推論|
|train|訓練模型|
|nnload|載入模型|
## Feature
* 擁有完整的parser與executor
* 在若要訓練模型則需要比較多的記憶體(以這個repo的資料集來說:256MB)，不過論純推論的話，記憶體將大幅減少，若只是跑個模型大約4MB即可(同樣以這個例子為例)
* 在每跑完一次指令(或模型的計算)會回收使用過的記憶體，重複利用
## Example
若要使用traindata_set.bin,可以去dataset資料夾底下，執行process_dataset.py
```shell=
yxsh (AI-Powered Shell) - Type 'quit' or 'exit' to leave
yxsh> train dataset/traindata_set.bin 700 1 0.001 10 64 32 # <dataset>  <input> <output> <lr> <epoch> <batch> <hidden>
yxsh> nnload model/shell_trained_model.bin
yxsh: model loaded [2 layers, input: 700, output: 1]
yxsh>  score dataset/traindata_set.bin                
Samples:  15731
RMSE:     0.1606
Est. Err: +/- 20.1 cycles
yxsh> infer dataset/engine_3_test.bin
PREDICT_RUL=93.5
PREDICT_PROB=0.2524
```
## Quick Start
```shell=
make
./yxsh
```
## Benchmark
### 在 raspberry pi zero 2wh 下:
```shell
yxsh (AI-Powered Shell) - Type 'quit' or 'exit' to leave
yxsh> nnload model/shell_trained_model.bin
yxsh: model loaded [2 layers, input: 700, output: 1]
yxsh>  score dataset/traindata_set.bin
Samples:  15731 RMSE:     0.1606
Est. Err: +/- 20.1 cycles
yxsh: batch latency: 1.9712 s | throughput: 7981 samples/sec
yxsh> infer dataset/engine_3_test.bin
PREDICT_RUL=93.5
PREDICT_PROB=0.2524
yxsh: inference latency: 229.64 us (0.2296 ms)
```
### 在 Ubuntu虛擬機下進行訓練(4核心):
利用pref工具，測量其在Page faults以及CPU使用率
```shell
yxsh> yxsh: training started [15731 samples, in: 700, out: 1]
yxsh: model saved to 'shell_trained_model.bin'

 Performance counter stats for './yxsh':
        13,7220.96 msec task-clock                       #    1.000 CPUs utilized             
               880      context-switches                 #    6.413 /sec                      
                 0      cpu-migrations                   #    0.000 /sec                      
                78      page-faults                      #    0.568 /sec                      
     <not supported>      cycles                                                                
     <not supported>      instructions                                                          
     <not supported>      branches                                                              
     <not supported>      branch-misses                                                         

     137.270693079 seconds time elapsed
     137.142991000 seconds user
     0.082973000 seconds sys
```
## Compare Pytorch
### Result
| 測試維度 (Metric) | PyTorch (PC VM) | **yxsh (PC VM)** | **yxsh (Zero 2 W)** | 關鍵觀察 (Insights) |
| :--- | :--- | :--- | :--- | :--- |
| **啟動開銷 (Startup)** | **1.04 sec** | **< 0.01 sec** | **< 0.01 sec** | **yxsh 快 100 倍**，徹底消除框架載入延遲。 |
| **推論延遲 (Latency)** | 81.39 $\mu s$ | **1.01 ms** | **1.89 ms** | 包含 Shell 解析與 I/O |
| **記憶體佔用 (RSS)** | 332.6 MB | **6.2 MB** | **5.6 MB** | **節省 98% 資源**，為 Zero 2 W 部署之關鍵。 |
| **磁碟干擾 (Faults)** | 55,680 (Minor) | **1,116 (Minor)** | **1,117 (Minor)** |  |
| **環境依賴 (Deps)** | 極高 (Python/pip) | **零依賴 (Pure C)** | **零依賴 (Pure C)** | 解決嵌入式系統常見的「依賴地獄」。 |


以下為實測數據，統一使用/usr/bin/time來測量
### Pure Benchmark (pure_bench.c)
下面數據記憶體大小較大的原因是因為一次性讀了全部的資料，因此需要花費較多的記憶體
```shell
amples:   15731
RMSE:      0.1606
Est. Err:  +/- 20.1 cycles
yxsh: pure inference latency: 0.0663 s | throughput: 237292 samples/sec
Avg latency per sample: 4.21 us
	Command being timed: "./bench"
	User time (seconds): 0.07
	System time (seconds): 0.09
	Percent of CPU this job got: 100%
	Elapsed (wall clock) time (h:mm:ss or m:ss): 0:00.16
	Average shared text size (kbytes): 0
	Average unshared data size (kbytes): 0
	Average stack size (kbytes): 0
	Average total size (kbytes): 0
	Maximum resident set size (kbytes): 309376
	Average resident set size (kbytes): 0
	Major (requiring I/O) page faults: 0
	Minor (reclaiming a frame) page faults: 76889
	Voluntary context switches: 1
	Involuntary context switches: 2
	Swaps: 0
	File system inputs: 0
	File system outputs: 0
	Socket messages sent: 0
	Socket messages received: 0
	Signals delivered: 0
	Page size (bytes): 4096
	Exit status: 0
```
### Ubuntu VM yxsh end-to-end benchemark (4核心)
```shell
yxsh (AI-Powered Shell) - Type 'quit' or 'exit' to leave
yxsh> yxsh: model loaded [2 layers, input: 700, output: 1]
PREDICT_RUL=93.5
PREDICT_PROB=0.2524
yxsh: inference latency: 1018.65 us (1.01865 ms)
	Command being timed: "./yxsh"
	User time (seconds): 0.00
	System time (seconds): 0.00
	Percent of CPU this job got: 50%
	Elapsed (wall clock) time (h:mm:ss or m:ss): 0:00.00
	Average shared text size (kbytes): 0
	Average unshared data size (kbytes): 0
	Average stack size (kbytes): 0
	Average total size (kbytes): 0
	Maximum resident set size (kbytes): 6272
	Average resident set size (kbytes): 0
	Major (requiring I/O) page faults: 0
	Minor (reclaiming a frame) page faults: 1116
	Voluntary context switches: 1
	Involuntary context switches: 0
	Swaps: 0
	File system inputs: 0
	File system outputs: 0
	Socket messages sent: 0
	Socket messages received: 0
	Signals delivered: 0
	Page size (bytes): 4096
	Exit status: 0
```
### Pytorch
此數據為了公平，因此不實際使用真正的dataset(為了忽略python的FILE I/O 的影響)
```shell
=== yxsh vs PyTorch: Baseline Benchmark ===
[System] Load PyTorch  time: 1.0430 sec

[nnload]model creation time: 1.37 ms

[score] Batch latency (15731 samples): 0.0107 s
[score] Throughput: 1468964 samples/sec

[infer] predicted value: 0.0262
[infer] Inference latency : 81.39 us (0.0814 ms)
	Command being timed: "python3 script/baseline.py"
	User time (seconds): 1.65
	System time (seconds): 0.21
	Percent of CPU this job got: 123%
	Elapsed (wall clock) time (h:mm:ss or m:ss): 0:01.51
	Average shared text size (kbytes): 0
	Average unshared data size (kbytes): 0
	Average stack size (kbytes): 0
	Average total size (kbytes): 0
	Maximum resident set size (kbytes): 332664
	Average resident set size (kbytes): 0
	Major (requiring I/O) page faults: 0
	Minor (reclaiming a frame) page faults: 55680
	Voluntary context switches: 17
	Involuntary context switches: 103
	Swaps: 0
	File system inputs: 0
	File system outputs: 0
	Socket messages sent: 0
	Socket messages received: 0
	Signals delivered: 0
	Page size (bytes): 4096
	Exit status: 0
```
### zero 2w
```shell
yxsh (AI-Powered Shell) - Type 'quit' or 'exit' to leave
yxsh> yxsh: model loaded [2 layers, input: 700, output: 1]
PREDICT_RUL=93.5
PREDICT_PROB=0.2524
yxsh: inference latency: 1899.26 us (1.89926 ms)
	Command being timed: "./yxsh"
	User time (seconds): 0.00
	System time (seconds): 0.01
	Percent of CPU this job got: 92%
	Elapsed (wall clock) time (h:mm:ss or m:ss): 0:00.01
	Average shared text size (kbytes): 0
	Average unshared data size (kbytes): 0
	Average stack size (kbytes): 0
	Average total size (kbytes): 0
	Maximum resident set size (kbytes): 5604
	Average resident set size (kbytes): 0
	Major (requiring I/O) page faults: 0
	Minor (reclaiming a frame) page faults: 1117
	Voluntary context switches: 1
	Involuntary context switches: 13
	Swaps: 0
	File system inputs: 0
	File system outputs: 0
	Socket messages sent: 0
	Socket messages received: 0
	Signals delivered: 0
	Page size (bytes): 4096
	Exit status: 0
```
zero 2w 上裝pytorch比較麻煩(受到硬體的限制，因此不測速度)
