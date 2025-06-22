import json
import matplotlib.pyplot as plt
import numpy as np
from tkinter import Tk, filedialog

# Hide tkinter root window
root = Tk()
root.withdraw()

# Select 3 files
file_paths = filedialog.askopenfilenames(
    title="Select 3 MPU6050 log files",
    filetypes=[("Text Files", "*.txt"), ("All Files", "*.*")]
)

if len(file_paths) != 3:
    print("Please select exactly 3 files.")
    exit()

# Read and plot each file
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

    time = np.arange(len(acc_x))  # Use numpy for easy shifting

    # Set width and shifts for grouped bars
    bar_width = 0.25
    offset1 = -bar_width
    offset2 = 0
    offset3 = bar_width

    # Plot each set in a new figure
    plt.figure(figsize=(15, 10))
    plt.suptitle(f'Data Visualization (Bar Graph) - File {idx + 1}', fontsize=16)

    # Acceleration Bar Graph
    plt.subplot(3, 1, 1)
    plt.bar(time + offset1, acc_x, width=bar_width, label='Acc X')
    plt.bar(time + offset2, acc_y, width=bar_width, label='Acc Y')
    plt.bar(time + offset3, acc_z, width=bar_width, label='Acc Z')
    plt.title('Acceleration')
    plt.xlabel('Time')
    plt.ylabel('m/s²')
    plt.legend()

    # Gyroscope Bar Graph
    plt.subplot(3, 1, 2)
    plt.bar(time + offset1, gyro_x, width=bar_width, label='Gyro X')
    plt.bar(time + offset2, gyro_y, width=bar_width, label='Gyro Y')
    plt.bar(time + offset3, gyro_z, width=bar_width, label='Gyro Z')
    plt.title('Gyroscope')
    plt.xlabel('Time')
    plt.ylabel('°/s')
    plt.legend()

    # Orientation Bar Graph
    plt.subplot(3, 1, 3)
    plt.bar(time + offset1, roll, width=bar_width, label='Roll')
    plt.bar(time + offset2, pitch, width=bar_width, label='Pitch')
    plt.bar(time + offset3, yaw, width=bar_width, label='Yaw')
    plt.title('Orientation (Roll, Pitch, Yaw)')
    plt.xlabel('Time')
    plt.ylabel('Degrees')
    plt.legend()

    plt.tight_layout(rect=[0, 0, 1, 0.96])  # Adjust layout to fit title

plt.show()
