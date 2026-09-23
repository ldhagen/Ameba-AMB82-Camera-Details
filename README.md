# Realtek AMB82-Mini RTSP Camera Firmware

Custom C++ Arduino firmware for the Realtek AMB82-Mini microcontroller. This project provides a robust, network-resilient 1080p RTSP video stream optimized for integration with **Frigate NVR**, featuring dynamic Over-The-Air (OTA) updates and a built-in telemetry dashboard.

## Key Features & Enhancements

*   **Zero-Delay Booting:** The camera pipeline and RTSP server initialize in under a second (`<160us`). Blocking boot delays have been eliminated to ensure immediate stream availability after a power cycle.
*   **WPA3 & PMF Compatibility:** Fully compatible with modern router security standards. The firmware successfully negotiates SAE authentication and Protected Management Frames (PMF) on 5GHz/2.4GHz networks.
*   **Passive Network Watchdog:** Relies on the Realtek SDK's internal auto-reconnect for minor signal drops, but enforces a strict 90-second timeout. If the WPA3 PMKSA cache locks up, the watchdog automatically executes a soft-reboot to clear the hardware RAM and restore the connection.
*   **Socket Persistence:** The RTSP and HTTP sockets bind to all available interfaces (`0.0.0.0`) and intentionally survive brief Wi-Fi disconnects. The firmware explicitly avoids re-binding sockets during an auto-recovery to prevent fatal LwIP port-collision crashes.
*   **Dynamic OTA Preparation:** Safely frees DMA memory for incoming firmware updates via a web endpoint, tearing down the video pipeline without requiring a time-consuming reboot sequence.
*   **Auto-Generating Versioning:** Uses C++ compiler macros (`__DATE__` and `__TIME__`) to automatically generate a unique, timestamped firmware version upon compilation, making successful OTA updates instantly verifiable.
*   **Custom Hostname:** Registers gracefully on the network as `AMB82-Mini` instead of the generic Realtek `lwip0` identifier.

## Web Telemetry Dashboard

The firmware hosts a lightweight HTTP web server on port `80`. Accessing the camera's local IP address (e.g., `http://192.168.4.104`) provides real-time diagnostics:
*   Current Firmware Timestamp
*   System Uptime
*   Wi-Fi Signal Strength (dBm)
*   Current RTSP Stream URL
*   Hardware Auto-Recovery Counters
*   Current Connection Status

## Over-The-Air (OTA) Update Guide

This firmware supports wireless flashing via a background OTA thread communicating with a local server (e.g., a Next.js application on `192.168.4.204:3000`). 

### Part 1: Generating & Locating the OTA Binary (`.bin`)
Before you can push an update over the air, you must compile your sketch into a raw `.bin` file and isolate the correct application image.

1.  **Compile:** Open your `.ino` file in the Arduino IDE, verify `Ameba_AMB82-MINI` is selected, and click **Sketch > Export compiled Binary** (`Ctrl+Alt+S`).
2.  **Identify the Correct File:** The Realtek compiler generates several files. You specifically need **`firmware.bin`** (the isolated OTA application). 
    *   **Do NOT use** `flash_ntz.bin` (this is the full USB flash image and will fail OTA).
    *   **Do NOT use** the raw `.ino` text file (this will result in a ~10KB file and a checksum error).
    *   The correct `firmware.bin` file will be approximately **1.5 MB to 2.5 MB** in size.
3.  **Locate the File (Linux / Hidden Toolchain Cache):** If the Arduino IDE fails to output the `.bin` into your sketch folder or `build/` directory, it is still safely stored in the Realtek toolchain cache. Open your terminal and copy it directly using this command:
    ```bash
    cp ~/.arduino15/packages/realtek/tools/ameba_pro2_tools/1.4.7/firmware.bin ~/firmware.bin
    ```
4.  **Stage the File:** Move `firmware.bin` into the **`public/`** folder of your Next.js project. Ensure your server route resolves directly to the file download (e.g., test `http://<SERVER_IP>:3000/firmware.bin` in your browser to verify it downloads the 1.5MB+ file and not a 404 HTML page).

### Part 2: Pushing the OTA Update
Because video streaming consumes significant DMA memory on the AMB82-Mini, you must free the hardware resources before pushing an update.

1.  **Open the Dashboard:** Navigate to the camera's IP address in your web browser.
2.  **Prepare the Hardware:** Click the blue **Prepare for OTA** button. 
    *   *What this does:* This endpoint (`POST /ota-prep`) cleanly stops the RTSP server, terminates the active Frigate stream, and shuts down the video pipeline. The DMA memory is now completely cleared for the incoming binary.
3.  **Push the Update:** Trigger the OTA deployment from your Next.js server UI.
4.  **Wait for Reboot:** The camera will download the binary, flash it to memory, and automatically reboot. This takes approximately 10-15 seconds.
5.  **Verify Success:** Refresh the camera's web dashboard. Look at the **Firmware** badge at the top of the page; the timestamp should exactly match the minute you exported the new binary from the Arduino IDE.

## Network Requirements & Tips

*   **2.4GHz Recommended:** While this firmware supports 5GHz WPA3 networks, 2.4GHz is strongly recommended for IoT cameras. 2.4GHz provides significantly better wall penetration and physical range, preventing the Realtek hardware from entering "zombie" disconnect states.
*   **Frigate Integration:** The RTSP stream is available at `rtsp://<CAMERA_IP>:554`. The server is configured to handle the aggressive multi-port binding behavior typical of Frigate NVR setups.
*   **SDK Version:** This code is optimized for the Realtek AmebaPro2 SDK **v4.1.0**.

## Manual Reboot

If the camera's video stream hangs but the web dashboard is still responsive, you can remotely restart the device by clicking the red **Reboot Camera** button (`POST /reset`) in the dashboard UI.
