import time
import sys
import numpy as np

import_start = time.perf_counter()
try:
    import torch
    import torch.nn as nn
except ImportError:
    sys.exit(1)
import_end = time.perf_counter()


class RULPredictor(nn.Module):
    def __init__(self):
        super().__init__()
        self.model = nn.Sequential(
            nn.Linear(700, 64),
            nn.Sigmoid(),
            nn.Linear(64, 32),
            nn.Sigmoid(),
            nn.Linear(32, 1),
        )

    def forward(self, x):
        return self.model(x)


def main():
    print("=== yxsh vs PyTorch: Baseline Benchmark ===")
    print(f"[System] Load PyTorch  time: {(import_end - import_start):.4f} sec\n")

    load_start = time.perf_counter()
    model = RULPredictor()
    model.eval()
    load_end = time.perf_counter()
    print(f"[nnload]model creation time: {(load_end - load_start) * 1000:.2f} ms\n")

    num_samples = 15731

    raw_data = np.fromfile("dataset/traindata_set.bin", dtype=np.float32)
    batch_data = torch.from_numpy(raw_data).view(15731, 701)
    batch_data = batch_data[:, :700]
    # batch_data = torch.randn(num_samples, 700)

    score_start = time.perf_counter()
    with torch.no_grad():
        _ = model(batch_data)
    score_end = time.perf_counter()

    score_time = score_end - score_start
    print(f"[score] Batch latency ({num_samples} samples): {score_time:.4f} s")
    print(f"[score] Throughput: {num_samples / score_time:.0f} samples/sec\n")

    single_data = torch.randn(1, 700)
    with torch.no_grad():
        _ = model(single_data)

        infer_start = time.perf_counter()
    with torch.no_grad():
        prediction = model(single_data)
    infer_end = time.perf_counter()

    infer_latency_us = (infer_end - infer_start) * 1_000_000
    print(f"[infer] predicted value: {prediction.item():.4f}")
    print(
        f"[infer] Inference latency : {infer_latency_us:.2f} us ({(infer_end - infer_start) * 1000:.4f} ms)"
    )


if __name__ == "__main__":
    main()
