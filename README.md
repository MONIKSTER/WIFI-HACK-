   **ESP8266 WiFi Setup Guide**

### *Setup Steps:*

1. **Install ESP8266 Board in Arduino IDE**

   - Open **Preferences** in Arduino IDE
   - Paste the following URL in **Additional Board Manager URLs**:
     ```
     http://arduino.esp8266.com/stable/package_esp8266com_index.json
     ```
   - Open **Board Manager** and install **ESP8266 Boards**
   - Select your ESP8266 board from the **Tools > Board** menu

2. **Install Required Libraries**

   - Open **Library Manager** in Arduino IDE
   - Install the following libraries:
     - **ESP8266WiFi** (for WiFi connectivity)
     - **DNSServer** (for handling domain name resolution)
     - **ESP8266WebServer** (for running a web server on ESP8266)

### **Code Summary:**

- The ESP8266 creates a **WiFi Access Point (AP)** with the following details:
  - **SSID:** `BOOMER`
  - **Password:** `99887766`
- Devices can connect to this network, and the ESP8266 can serve web pages or handle requests using `ESP8266WebServer`.
- The **DNSServer** library can be used to redirect DNS queries, useful for **captive portals** (like public WiFi login pages).
- The **WebServer** can be used to host a webpage, collect user data, or display information.


### **Features:**

- Creates a WiFi hotspot named `BOOMER` with password `99887766`.
- Hosts a simple web server that responds with a welcome message.
- Clients can connect to `http://192.168.4.1/` to access the page.

