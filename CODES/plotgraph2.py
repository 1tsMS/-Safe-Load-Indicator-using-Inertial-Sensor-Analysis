import pandas as pd
import matplotlib.pyplot as plt
import json
import os

# ========== CONFIGURE FILE PATH ==========
file_path = 'E:\College\projects\Crane-iacs\CODES\LOGS\new\Load place\mpu6050_log_4.txt'  # Change this to your file

# ========== LOAD & PARSE DATA ==========
def load_mpu6050_data(file_path):
    with open(file_path, 'r') as file:
        lines = file.readlines()
    records = [json.loads(line.strip()) for line in lines]
    df = pd.DataFrame([{
        'Ax': r['acceleration']['x'],
        'Ay': r['acceleration']['y'],
        'Az': r['acceleration']['z'],
        'Roll': r['roll'],
        'Pitch': r['pitch'],
        'Yaw': r['yaw']
    } for r in records])
    return df

# ========== CALCULATE JERK ==========
def compute_jerk(df):
    df['Jerk_Az'] = [0] + list(df['Az'].diff().fillna(0))
    return df

# ========== PLOTTING ==========
def plot_accel_and_jerk(df, title='MPU6050: Z-Axis Acceleration & Jerk'):
    time = list(range(len(df)))
    plt.figure(figsize=(10, 5))
    plt.plot(time, df['Az'], label='Z-Acceleration', color='navy', marker='o')
    plt.plot(time, df['Jerk_Az'], label='Z-Jerk (ΔAz)', color='tomato')
    plt.title(title)
    plt.xlabel("Sample")
    plt.ylabel("Value")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.show()

# ========== MAIN EXECUTION ==========
df = load_mpu6050_data(file_path)
df = compute_jerk(df)
plot_accel_and_jerk(df, f"{os.path.basename(file_path)} - Load Set Down Phase")
