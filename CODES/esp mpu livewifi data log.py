
import requests
import time
import os

# ESP32 server details
ESP32_IP = "192.168.134.175"  # Replace with your ESP32's IP address
ESP32_PORT = 80
LOG_DIR = "E:\College\projects\Crane-iacs\CODES"  # Change this to your preferred log directory

def fetch_data():
    try:
        response = requests.get(f"http://{ESP32_IP}:{ESP32_PORT}/data")
        if response.status_code == 200:
            return response.text.strip()
    except requests.RequestException as e:
        print(f"Error fetching data: {e}")
    return None

def delete_logs_on_esp():
    try:
        response = requests.get(f"http://{ESP32_IP}:{ESP32_PORT}/delete_log")
        if response.status_code == 200 and "LOG_DELETED" in response.text:
            print("Logs deleted on ESP32 and reset.")
    except requests.RequestException as e:
        print(f"Error deleting logs: {e}")

def get_new_log_filename():
    if not os.path.exists(LOG_DIR):
        os.makedirs(LOG_DIR)
    existing_files = [f for f in os.listdir(LOG_DIR) if f.startswith("mpu6050_log_") and f.endswith(".txt")]
    numbers = sorted([int(f.split("_")[-1].split(".")[0]) for f in existing_files if f.split("_")[-1].split(".")[0].isdigit()])
    new_number = numbers[-1] + 1 if numbers else 1
    return os.path.join(LOG_DIR, f"mpu6050_log_{new_number}.txt")

def start_logging():
    log_file = get_new_log_filename()
    print(f"Logging live MPU6050 data to {log_file}... Press ENTER to stop.")
    
    with open(log_file, "w", encoding="utf-8") as file:
        try:
            while True:
                data = fetch_data()
                if data:
                    print(f"Received: {data}")
                    file.write(data + "\n")
                    file.flush()
                time.sleep(1)  # Fetch data every second
        except KeyboardInterrupt:
            print(f"\nLogging stopped. Data saved to {log_file}")

def main_menu():
    while True:
        print("\nMenu:")
        print("1. Start logging MPU6050 data")
        print("2. Delete logs on ESP32 and reset count")
        print("3. Exit")

        choice = input("Enter your choice: ").strip()

        if choice == "1":
            start_logging()
        elif choice == "2":
            delete_logs_on_esp()
        elif choice == "3":
            print("Exiting program.")
            break
        else:
            print("Invalid choice. Please enter 1, 2, or 3.")

if __name__ == "__main__":
    main_menu()
