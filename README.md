Here is a comprehensive `README.md` file that documents all the features, settings, and recovery mechanisms currently built into your sketch.

Markdown

`# AMB82-Mini RTSP Video Streamer & Watchdog`  `This project is a robust, self-healing RTSP video streaming firmware for the AMB82-Mini microcontroller. It includes a built-in web dashboard, Over-The-Air (OTA) update capabilities, and an aggressive network watchdog designed to keep remote cameras online without physical intervention.`  `## 🚀 Features`  `*``   ``**RTSP Video Streaming:**``  Streams HD H.264 video at 5 FPS directly from the AMB82-Mini camera sensor. ` `*``   ``**Over-The-Air (OTA) Updates:**``  Dedicated background thread for receiving firmware flashes via a Next.js UI, featuring a 50-second "Safe Window" on boot to ensure the camera DMA doesn't interfere with flash memory writes. ` `*``   ``**Auto-Healing Network Stack:**``  Actively monitors the WiFi connection. If the connection drops, it forces a network stack reset and attempts to reconnect every 10 seconds. ` `*``   ``**Hard-Reboot Watchdog:**``  If the device remains entirely disconnected from WiFi for 15 consecutive minutes, it triggers a hardware-level  ``` `sys_reset()` ```  to clear any deep software or router-level hangs. ` `*``   ``**Diagnostic Web Dashboard:**``  Hosts a lightweight web server on port 80 to monitor camera health in real-time. `  `## 📊 Web Dashboard Capabilities`  `You can access the control panel by navigating to the camera's local IP address in a web browser. The dashboard automatically refreshes every 5 seconds and provides:`  `*``   ``**Dynamic Firmware Versioning:**``  Displays the current software build (e.g.,  ``` `v6.7.3` ```).` `*``   ``**System Uptime:**``  Tracks days, hours, minutes, and seconds since the last boot. ` `*``   ``**Network Telemetry:**``  Displays the connected SSID and current Signal Strength (RSSI). ` `*``   ``**RTSP URL:**``  Provides the direct link to plug into VLC, Frigate, or Home Assistant. ` `*``   ``**Auto-Recovery Logs:**``  Tracks how many times the camera successfully healed a broken WiFi connection since it last booted. ` `*``   ``**Live Status:**``  Shows whether the camera is ONLINE or OFFLINE (including current reconnection attempt counts). ` `*``   ``**Manual Reboot:**``  A UI button to remotely trigger a safe hardware reset. `  `## ⚙️ Configuration`  ` Before compiling and flashing this firmware, update the following variables at the top of the  ``` `.ino` ```  file to match your environment: `  ```` ```cpp ```` `// 1. Network Credentials` `char ssid[] = "YOUR_SSID_HERE";` ` char pass[] = "YOUR_PASSWORD_HERE";  `  `// 2. Firmware Version (Update this before exporting your OTA .bin file!)` `const String FIRMWARE_VERSION = "vX.X.X";`  `// 3. OTA Server Configuration` `int otaPort = 3000;` `char* otaServerIp = "192.168.X.X"; // IP of the machine hosting the Next.js OTA UI` 
## **🛠️ The OTA "Safe Window"**
Flashing firmware to the AMB82-Mini while the camera is actively writing to memory via DMA can cause fatal crashes. To prevent this, the firmware uses a **Safe Window** approach:

1. On boot, the network connects and the OTA thread starts immediately.

1. The main thread halts for **50 seconds**.

1. During this time, the camera hardware is completely OFF.

1. **This is the time to click "Start OTA" in your web UI.**

1. If no OTA is triggered, the 50 seconds expire, the video pipeline boots, and standard RTSP streaming begins.

## **🛡️ How the Watchdog Works**
Microcontrollers can occasionally experience "zombie" network states where the router drops the device, but the device thinks it is still connected. This code fights that using a dual-layered approach:

1. **Layer 1 (The Soft Fix):** If `WiFi.status()` returns disconnected, the loop immediately triggers a `WiFi.disconnect()` to explicitly clear the hung state, waits 200ms, and tries `WiFi.begin()`. It repeats this every 10 seconds.

1. **Layer 2 (The Hard Fix):** If 15 minutes elapse and Layer 1 has not successfully reconnected, the system assumes a critical failure has occurred (either in the IP stack or video pipeline starvation) and triggers `sys_reset()`.

## **📦 Dependencies**
This sketch requires the official Realtek Ameba board packages and relies on the following built-in libraries:

- `WiFi.h`

- `StreamIO.h`

- `VideoStream.h`

- `RTSP.h`

- `OTA.h`

- `sys_api.h`


