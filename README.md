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
在 raspberry pi zero 2wh 下:
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
在 Ubuntu虛擬機下進行訓練(單核心):
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
