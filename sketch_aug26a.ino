
#include "WiFi.h"
#include "StreamIO.h"
#include "VideoStream.h"
#include "RTSP.h"
#include "OTA.h"      
#include "sys_api.h"  

#define CHANNEL 0

VideoSetting config(VIDEO_HD, 5, VIDEO_H264, 0); 
RTSP rtsp;
StreamIO videoStreamer(1, 1);
WiFiServer server(80); 

char ssid[] = "dd-wrt-50_EXT";
char pass[] = "xxxxxx"; 

// Update this before every OTA export!
const String FIRMWARE_VERSION = "v6.7.3";
// --- Ameba OTA HTTP Settings ---
OTA ota; 
int otaPort = 3000;
char* otaServerIp = "192.168.4.204"; 
// -------------------------------

// --- Offline Watchdog & Reconnect Settings ---
unsigned long offlineStartTime = 0;
unsigned long lastReconnectAttempt = 0;
bool isOffline = false;

// Telemetry Counters
int reconnectAttempts = 0;
int totalRecoveryEvents = 0;

const unsigned long OFFLINE_REBOOT_TIMEOUT = 15 * 60 * 1000; 
const unsigned long RECONNECT_INTERVAL = 10000;              
// ---------------------------------------------

void setup() {
    Serial.begin(115200);
    
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(2000);
        Serial.print(".");
    }
    Serial.println("\nConnected!");
    server.begin(); 

    ota.start_OTA_threads(otaPort, otaServerIp); 
    Serial.println("OTA Thread Started.");

    Serial.println("\n==============================================");
    Serial.println("🛑 OTA SAFE WINDOW ACTIVE 🛑");
    Serial.println("Camera hardware is physically OFF to free the DMA.");
    Serial.println("Go to your Next.js UI and click 'Start OTA' NOW!");
    Serial.println("==============================================\n");

    for(int i = 50; i > 0; i--) {
        Serial.print(i); Serial.println(" seconds until camera boots...");
        delay(1000);
    }

    Serial.println("\nBooting Video Pipeline...");
    config.setBitrate(384 * 1024); 
    Camera.configVideoChannel(CHANNEL, config);
    Camera.videoInit();

    rtsp.configVideo(config);
    rtsp.begin();

    videoStreamer.registerInput(Camera.getStream(CHANNEL));
    videoStreamer.registerOutput(rtsp);
    videoStreamer.begin();
    Camera.channelBegin(CHANNEL);
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        unsigned long currentMillis = millis();

        if (!isOffline) {
            isOffline = true;
            offlineStartTime = currentMillis;
            reconnectAttempts = 0; 
            Serial.println("WiFi disconnected. Starting 15-minute watchdog...");
        } 
        else if (currentMillis - offlineStartTime >= OFFLINE_REBOOT_TIMEOUT) {
            Serial.println("Offline for 15 minutes. Rebooting now...");
            delay(1000);
            sys_reset(); 
        }

        if (currentMillis - lastReconnectAttempt >= RECONNECT_INTERVAL) {
            lastReconnectAttempt = currentMillis;
            reconnectAttempts++;
            
            Serial.print("Attempting to reconnect... (Attempt ");
            Serial.print(reconnectAttempts);
            Serial.println(")");
            
            // FIX: Force clear the WiFi state before attempting to bind again
            WiFi.disconnect();
            delay(200); // Brief pause to let the hardware settle
            WiFi.begin(ssid, pass); 
        }
    } else {
        if (isOffline) {
            isOffline = false;
            totalRecoveryEvents++;
            Serial.print("WiFi reconnected! It took ");
            Serial.print(reconnectAttempts);
            Serial.println(" attempts. Watchdog cancelled.");
            reconnectAttempts = 0; 
        }
        
        handleWebDebugger();
    }
}

void handleWebDebugger() {
    WiFiClient client = server.available();
    if (client) {
        String currentLine = "";
        String firstLine = "";
        bool isFirstLine = true;
        unsigned long timeoutTime = millis() + 150; 

        while (client.connected() && millis() < timeoutTime) {
            if (client.available()) {
                char c = client.read();
                timeoutTime = millis() + 150; 
                
                if (isFirstLine) {
                    firstLine += c;
                    if (c == '\n') isFirstLine = false;
                }

                if (c == '\n') {
                    if (currentLine.length() == 0) {
                        
                        if (firstLine.indexOf("POST /reset") >= 0) {
                            client.println("HTTP/1.1 200 OK");
                            client.println("Content-type:text/html");
                            client.println("Connection: close");
                            client.println();
                            client.println("<html><head><meta http-equiv='refresh' content='15;url=/'></head>");
                            client.println("<body style='font-family:sans-serif; background:#222; color:#eee; padding:20px;'>");
                            client.println("<h1>Rebooting AMB82-Mini...</h1>");
                            client.stop();
                            delay(1000); 
                            sys_reset(); 
                            break;
                        }

                        unsigned long uptimeSeconds = millis() / 1000;
                        int days = uptimeSeconds / 86400;
                        int hours = (uptimeSeconds % 86400) / 3600;
                        int minutes = ((uptimeSeconds % 86400) % 3600) / 60;
                        int seconds = uptimeSeconds % 60;

                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println("Connection: close"); 
                        client.println();
                        
                        client.println("<html><head><title>Amb82 Dashboard</title>");
                        client.println("<meta http-equiv='refresh' content='5'></head>"); 
                        client.println("<body style='font-family:sans-serif; background:#222; color:#eee; padding:20px;'>");
                        client.println("<h1>AMB82-Mini Dashboard</h1>");
                        
                        client.println("<div style='background-color: #ffd700; color: #000; padding: 10px; border-radius: 5px; margin-bottom: 20px;'>");
                        client.print("<strong>Firmware Version: ");
                        client.print(FIRMWARE_VERSION); 
                        client.println("</strong></div>");
                        
                        client.print("<p><b>Uptime:</b> ");
                        client.print(days); client.print("d ");
                        client.print(hours); client.print("h ");
                        client.print(minutes); client.print("m ");
                        client.print(seconds); client.println("s</p>");

                        client.print("<p><b>Signal Strength:</b> ");
                        client.print(WiFi.RSSI());
                        client.println(" dBm</p>");
                        
                        client.print("<p><b>Connected Network:</b> ");
                        client.print(WiFi.SSID());
                        client.println("</p>");
                        
                        client.print("<p><b>RTSP URL:</b> rtsp://");
                        client.print(WiFi.localIP());
                        client.println(":554</p>");

                        // New Telemetry Section
                        client.println("<hr style='border:1px solid #444; margin:20px 0;'>");
                        client.println("<h3>Network Diagnostics</h3>");
                        client.print("<p><b>Total Auto-Recoveries:</b> ");
                        client.print(totalRecoveryEvents);
                        client.println("</p>");
                        
                        if (isOffline) {
                            client.print("<p style='color: #d9534f;'><b>Current Status:</b> OFFLINE (Attempting Reconnect...)</p>");
                            client.print("<p><b>Current Attempts:</b> ");
                            client.print(reconnectAttempts);
                            client.println("</p>");
                        } else {
                            client.println("<p style='color: #5cb85c;'><b>Current Status:</b> ONLINE</p>");
                        }
                        
                        client.println("<hr style='border:1px solid #444; margin:20px 0;'>");
                        client.println("<form action='/reset' method='POST' onsubmit=\"return confirm('Are you sure you want to reboot the camera?');\">");
                        client.println("<button type='submit' style='background:#d9534f; color:white; padding:10px 15px; border:none; border-radius:4px; cursor:pointer; font-weight:bold;'>");
                        client.println("Reboot Camera</button></form>");
                        
                        client.println("</body></html>");
                        break;
                    } else {
                        currentLine = "";
                    }
                } else if (c != '\r') {
                    currentLine += c;
                }
            }
        }
        client.stop(); 
    }
}
