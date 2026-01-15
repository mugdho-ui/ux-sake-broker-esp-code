// const mqtt = require('mqtt');

// // ---------- MQTT CONFIG ----------
// const BROKER = 'mqtt://bdtmp.ultra-x.jp';
// const PORT = 1883;

// const USERNAME = 'admin';
// const PASSWORD = 'StrongPassword123';

// const SEN1_TOPIC = 'level';

// // ---------- MQTT OPTIONS ----------
// const options = {
//   port: PORT,
//   clientId: 'js_subscriber_001',
//   username: USERNAME,
//   password: PASSWORD,
//   clean: true,
//   keepalive: 60,
// };

// // ---------- CONNECT ----------
// const client = mqtt.connect(BROKER, options);

// client.on('connect', () => {
//   console.log('[subscriber] Connected to broker ✅');

//   client.subscribe(SEN1_TOPIC, { qos: 1 }, (err) => {
//     if (err) {
//       console.error('[subscriber] Subscribe error:', err);
//     } else {
//       console.log(`[subscriber] Subscribed to topic: ${SEN1_TOPIC}`);
//     }
//   });
// });

// // ---------- MESSAGE ----------
// client.on('message', (topic, message) => {
//   console.log(
//     `[subscriber] RECV ${topic} | payload=${message.toString()} | time=${new Date()}`
//   );
// });

// // ---------- ERROR ----------
// client.on('error', (err) => {
//   console.error('[subscriber] MQTT Error:', err);
// });

// // ---------- CLOSE ----------
// client.on('close', () => {
//   console.log('[subscriber] Connection closed');
// });
const mqtt = require('mqtt');

// ---------- MQTT CONFIG ----------
const BROKER = 'mqtt://bdtmp.ultra-x.jp';
const PORT = 1883;

const USERNAME = 'admin';
const PASSWORD = 'StrongPassword123';

const SEN1_TOPIC = 'level';

// Generate unique client ID to avoid conflicts
const CLIENT_ID = `js_level_${Date.now()}_${Math.random().toString(16).substr(2, 8)}`;

// ---------- MQTT OPTIONS ----------
const options = {
  port: PORT,
  clientId: CLIENT_ID,
  username: USERNAME,
  password: PASSWORD,
  clean: true,                    // Clean session
  keepalive: 30,                  // Shorter keepalive (safer with broker's 60s heartbeat)
  reconnectPeriod: 5000,          // Wait 5s before reconnecting
  connectTimeout: 30000,          // 30s connection timeout
  resubscribe: true,              // Auto-resubscribe on reconnect
  protocolVersion: 4,             // MQTT 3.1.1
  reschedulePings: true,          // Automatically reschedule pings
};

// ---------- CONNECT ----------
console.log(`[subscriber] Connecting with client ID: ${CLIENT_ID}`);
const client = mqtt.connect(BROKER, options);

// ---------- CONNECTION ----------
client.on('connect', () => {
  console.log('[subscriber] Connected to broker ✅');

  client.subscribe(SEN1_TOPIC, { qos: 1 }, (err) => {
    if (err) {
      console.error('[subscriber] Subscribe error:', err);
    } else {
      console.log(`[subscriber] Subscribed to topic: ${SEN1_TOPIC}`);
    }
  });
});

// ---------- MESSAGE ----------
client.on('message', (topic, message) => {
  console.log(
    `[subscriber] RECV ${topic} | payload=${message.toString()} | time=${new Date()}`
  );
});

// ---------- RECONNECT ----------
client.on('reconnect', () => {
  console.log('[subscriber] Attempting to reconnect...');
});

// ---------- OFFLINE ----------
client.on('offline', () => {
  console.log('[subscriber] Client went offline');
});

// ---------- ERROR ----------
client.on('error', (err) => {
  console.error('[subscriber] MQTT Error:', err.message);
});

// ---------- CLOSE ----------
client.on('close', () => {
  console.log('[subscriber] Connection closed');
});

// ---------- GRACEFUL SHUTDOWN ----------
process.on('SIGINT', () => {
  console.log('\n[subscriber] Shutting down gracefully...');
  client.end(false, () => {
    console.log('[subscriber] Disconnected');
    process.exit(0);
  });
});

process.on('SIGTERM', () => {
  console.log('\n[subscriber] Received SIGTERM, shutting down...');
  client.end(false, () => {
    console.log('[subscriber] Disconnected');
    process.exit(0);
  });
});

// Keep process alive
console.log('[subscriber] MQTT client started');