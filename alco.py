import paho.mqtt.client as mqtt
import datetime

BROKER = "bdtmp.ultra-x.jp"  # বা তোমার PC-এর IP
PORT = 1883  # Secure MQTT TLS port

USERNAME = "admin"  # .env এ যেটা সেট করেছো
PASSWORD = "StrongPassword123"
SEN1_TOPIC = "CO2"




# ---------- MQTT Callbacks ----------
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("[subscriber] Connected to broker ✅ (TLS)")

        client.subscribe(SEN1_TOPIC, qos=1)

    else:
        print(f"[subscriber] Failed to connect, rc={rc}")


def on_message(client, userdata, msg):
    print(f"[subscriber] RECV {msg.topic} | payload={msg.payload.decode()} | time={datetime.datetime.now()}")


# ---------- MQTT Client Setup ----------
client = mqtt.Client(client_id="py_subscriber_001", clean_session=True)
client.username_pw_set(USERNAME, PASSWORD)



client.on_connect = on_connect
client.on_message = on_message

# Connect to TLS broker
client.connect(BROKER, PORT, keepalive=60)
client.loop_start()  # starts network loop in background



try:
    while True:
        import time

        time.sleep(1)
except KeyboardInterrupt:
    print("\n[main] stopping...")
finally:
    client.loop_stop()
    client.disconnect()
