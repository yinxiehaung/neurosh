import numpy as np
import os

SELECTED_SENSORS = [6, 7, 8, 11, 12, 13, 15, 16, 17, 18, 19, 21, 24, 25]
NUM_FEATURES = len(SELECTED_SENSORS)
WINDOW_SIZE = 50
MAX_RUL_LIMIT = 125.0


def get_normalization_params(filename):
    data = np.loadtxt(filename)
    sensors = data[:, SELECTED_SENSORS]
    return sensors.min(axis=0), sensors.max(axis=0)


def process_train_data():
    filename = "train_FD001.txt"
    if not os.path.exists(filename):
        print(f"找不到檔案: {filename}")
        return

    data = np.loadtxt(filename)
    s_min, s_max = get_normalization_params(filename)

    dataset = []
    for engine_id in np.unique(data[:, 0]):
        engine_data = data[data[:, 0] == engine_id]
        sensors = (engine_data[:, SELECTED_SENSORS] - s_min) / (s_max - s_min + 1e-9)

        total_cycles = len(engine_data)
        for i in range(total_cycles - WINDOW_SIZE + 1):
            window = sensors[i : i + WINDOW_SIZE].flatten()
            current_cycle = i + WINDOW_SIZE
            remaining_life = total_cycles - current_cycle
            raw_label = max(0.0, (MAX_RUL_LIMIT - remaining_life) / MAX_RUL_LIMIT)
            label = np.clip(raw_label, 0.05, 0.95)

            row = np.append(window, np.float32(label))
            dataset.append(row)

    train_matrix = np.array(dataset, dtype=np.float32)
    train_matrix.tofile("../traindata_set.bin")
    print(f"訓練集處理完成，形狀: {train_matrix.shape}")


def process_test_data(engine_ids=[1, 3, 5]):
    s_min, s_max = get_normalization_params("train_FD001.txt")
    test_data = np.loadtxt("test_FD001.txt")

    for eid in engine_ids:
        engine_data = test_data[test_data[:, 0] == eid]
        sensors = (engine_data[:, SELECTED_SENSORS] - s_min) / (s_max - s_min + 1e-9)

        if len(sensors) >= WINDOW_SIZE:
            faulty_window = sensors[-WINDOW_SIZE:].flatten().astype(np.float32)
            faulty_window.tofile(f"../engine_{eid}_test.bin")
            print(f"引擎 {eid} 測試資料導出成功")


if __name__ == "__main__":
    process_train_data()
    process_test_data([1, 3, 5])
