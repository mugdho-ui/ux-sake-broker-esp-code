import paho.mqtt.client as mqtt
import time

BROKER = "bdtmp.ultra-x.jp"
PORT = 1883

USERNAME = "admin"
PASSWORD = "StrongPassword123"

TOPIC = "text"   # test topic

client = mqtt.Client(client_id="quick_publisher")
client.username_pw_set(USERNAME, PASSWORD)

client.connect(BROKER, PORT, keepalive=60)
client.loop_start()

try:
    while True:
        msg = input("[PUB] Enter message: ")
        if msg.strip() == "":
            continue
        client.publish(TOPIC, msg, qos=1)
        print(f"[PUB] Sent → {msg}")
except KeyboardInterrupt:
    print("\n[PUB] Stopped")
finally:
    client.loop_stop()
    client.disconnect()
