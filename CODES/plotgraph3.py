import json
import matplotlib.pyplot as plt
from tkinter import Tk, filedialog

# Hide the root tkinter window
root = Tk()
root.withdraw()

# File selection dialog
file_paths = filedialog.askopenfilenames(
    title="Select 3 MPU6050 log files",
    filetypes=[("Text Files", "*.txt"), ("All Files", "*.*")]
)

if len(file_paths) != 3:
    print("Please select exactly 3 files.")
    exit()

# Read and plot each file in its own figure
for idx, path in enumerate(file_paths):
    acc_x, acc_y, acc_z = [], [], []
    gyro_x, gyro_y, gyro_z = [], [], []
    roll, pitch, yaw = [], [], []

    with open(path, "r") as f:
        for line in f:
            data = json.loads(line.strip())
            acc = data["acceleration"]
            gyro = data["gyroscope"]
            acc_x.append(acc["x"])
            acc_y.append(acc["y"])
            acc_z.append(acc["z"])
            gyro_x.append(gyro["x"])
            gyro_y.append(gyro["y"])
            gyro_z.append(gyro["z"])
            roll.append(data["roll"])
            pitch.append(data["pitch"])
            yaw.append(data["yaw"])

    time = list(range(len(acc_x)))

    # Create a new figure window for this file
    plt.figure(figsize=(15, 10))
    plt.suptitle(f'Data Visualization - File {idx + 1}', fontsize=16)

    # Acceleration scatter plot
    plt.subplot(3, 1, 1)
    plt.scatter(time, acc_x, label='Acc X', s=10)
    plt.scatter(time, acc_y, label='Acc Y', s=10)
    plt.scatter(time, acc_z, label='Acc Z', s=10)
    plt.title('Acceleration')
    plt.xlabel('Time')
    plt.ylabel('m/s²')
    plt.legend()

    # Gyroscope scatter plot
    plt.subplot(3, 1, 2)
    plt.scatter(time, gyro_x, label='Gyro X', s=10)
    plt.scatter(time, gyro_y, label='Gyro Y', s=10)
    plt.scatter(time, gyro_z, label='Gyro Z', s=10)
    plt.title('Gyroscope')
    plt.xlabel('Time')
    plt.ylabel('°/s')
    plt.legend()

    # Orientation scatter plot
    plt.subplot(3, 1, 3)
    plt.scatter(time, roll, label='Roll', s=10)
    plt.scatter(time, pitch, label='Pitch', s=10)
    plt.scatter(time, yaw, label='Yaw', s=10)
    plt.title('Orientation (Roll, Pitch, Yaw)')
    plt.xlabel('Time')
    plt.ylabel('Degrees')
    plt.legend()

# Show all figure windows
plt.show()
