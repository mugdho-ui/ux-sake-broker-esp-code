import paho.mqtt.client as mqtt
import time

BROKER = "bdtmp.ultra-x.jp"
PORT = 1883
USERNAME = "admin"
PASSWORD = "StrongPassword123"

# Topics
TEMP_THRESHOLD = "threshold/temp"
DISTANCE_THRESHOLD = "threshold/distance"



def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ Connected to MQTT broker!")
    else:
        print(f"❌ Connection failed, rc={rc}")


def on_publish(client, userdata, mid):
    print(f"📤 Message published (mid: {mid})")


# Setup client
client = mqtt.Client(client_id="py_threshold_controller")
client.username_pw_set(USERNAME, PASSWORD)
client.on_connect = on_connect
client.on_publish = on_publish

# Connect
print(f"Connecting to {BROKER}:{PORT}...")
client.connect(BROKER, PORT, keepalive=60)
client.loop_start()

# Wait for connection
time.sleep(2)

print("\nCommands:")
print("temp <value>     - Set temperature threshold")
print("distance <value> - Set distance threshold")
print("fan <value>      - Set fan speed 0-255")
print("quit             - Exit\n")

while True:
    cmd = input("> ").strip().lower()

    if cmd == "quit":
        break

    parts = cmd.split()
    if len(parts) != 2:
        print("❌ Invalid! Use: temp 35 OR distance 15 OR fan 200")
        continue

    command, value = parts

    if command == "temp":
        result = client.publish(TEMP_THRESHOLD, value, qos=1)
        print(f"✅ Published to {TEMP_THRESHOLD}: {value} (mid: {result.mid})")
    elif command == "distance":
        result = client.publish(DISTANCE_THRESHOLD, value, qos=1)
        print(f"✅ Published to {DISTANCE_THRESHOLD}: {value} (mid: {result.mid})")

    else:
        print("❌ Unknown command! Use: temp, distance, or fan")

    time.sleep(0.1)  # Small delay

client.loop_stop()
client.disconnect()
print("👋 Disconnected")