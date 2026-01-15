import paho.mqtt.client as mqtt
import datetime

BROKER = "bdtmp.ultra-x.jp"
PORT = 1883

USERNAME = "admin"
PASSWORD = "StrongPassword123"

# Test topics (subscribe all you want)
TOPICS = [
    ("ESP2", 1),
    ("ds18", 1),
    ("text", 1),
    ("CO2", 1),
    ("ESP", 1),
    ("sugarT", 1),
    ("level", 1),
]

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("[SUB] Connected to broker ✅")
        for topic, qos in TOPICS:
            client.subscribe(topic, qos)
            print(f"[SUB] Subscribed to: {topic}")
    else:
        print(f"[SUB] Connection failed ❌ rc={rc}")

def on_message(client, userdata, msg):
    print(f"[SUB] {msg.topic} | {msg.payload.decode()} | {datetime.datetime.now()}")

client = mqtt.Client(client_id="quick_subscriber")
client.username_pw_set(USERNAME, PASSWORD)

client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER, PORT, keepalive=60)
client.loop_forever()
