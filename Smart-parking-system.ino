#include <WiFi.h>
#include <ThingSpeak.h>

// Ultrasonic Sensor Pins
#define TRIG_PIN 5
#define ECHO_PIN 18

// LED Pins
#define GREEN_LED 2
#define RED_LED 4
#define WHITE_LED 14  // Yellow (White LED) for booking feature

// Buzzer Pin
#define BUZZER_PIN 27

// WiFi Credentials
const char* ssid = "OnePlus Nord 3 5G";           // Replace with your WiFi SSID
const char* password = "oneplus+";               // Replace with your WiFi password

// ThingSpeak Configuration
unsigned long channelID = 2777773;               // Replace with your ThingSpeak Channel ID
const char* writeAPIKey = "VMOL0FACFPHGML3F";    // Replace with your ThingSpeak Write API Key

WiFiServer server(80);

bool isOccupied = false; // Parking slot status
bool isBooked = false;   // Booking status

WiFiClient client;

unsigned long lastUpdateTime = 0; // Track time of the last ThingSpeak update
const unsigned long updateInterval = 20000; // Update ThingSpeak every 20 seconds

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(WHITE_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(WHITE_LED, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  ThingSpeak.begin(client);
  server.begin();
}

void loop() {
  long duration, distance;

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration * 0.034 / 2;

  isOccupied = (distance < 10 && distance > 0);

  // LED Logic
  if (isOccupied && !isBooked) {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(WHITE_LED, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  } else if (isBooked) {
    digitalWrite(WHITE_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);
    if (isOccupied) {
      digitalWrite(BUZZER_PIN, HIGH);
    } else {
      digitalWrite(BUZZER_PIN, LOW);
    }
  } else {
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(WHITE_LED, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }

  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    if (request.indexOf("/book") != -1) {
      isBooked = true;
    } else if (request.indexOf("/unbook") != -1) {
      isBooked = false;
    }

    if (request.indexOf("/status") != -1) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.print("{\"occupied\":");
      client.print(isOccupied ? "true" : "false");
      client.print(", \"booked\":");
      client.print(isBooked ? "true" : "false");
      client.println("}");
    } else {
      sendHTMLPage(client);
    }
    client.stop();
  }

  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdateTime >= updateInterval) {
    updateThingSpeak();
    lastUpdateTime = currentMillis;
  }
  delay(50);
}

void sendHTMLPage(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE html>");
  client.println("<html>");
  client.println("<head>");
  client.println("<title>Smart Parking</title>");
  client.println("<style>");
  client.println("body { font-family: 'Arial', sans-serif; text-align: center; padding: 20px; background-image: url('https://i.pinimg.com/originals/d9/7d/0f/d97d0f2675480191f9a2b126d0a1145f.gif'); background-size: cover; background-position: center; }");
  client.println("h1 { color: black; font-size: 48px; font-weight: bold; text-shadow: 2px 2px 10px rgba(0, 0, 0, 0.7); }");
  client.println(".status { font-size: 30px; margin-top: 20px; color: #fff; font-weight: bold; text-shadow: 1px 1px 6px rgba(0, 0, 0, 0.5); }");
  client.println("button { margin: 10px; padding: 15px 30px; font-size: 20px; cursor: pointer; background-color: rgba(0, 0, 0, 0.7); color: #fff; border: none; border-radius: 10px; transition: background-color 0.3s, opacity 0.3s; }");
  client.println("button:hover { background-color: rgba(255, 255, 255, 0.3); opacity: 0.8; }");
  client.println("</style>");
  client.println("</head>");
  client.println("<body>");
  client.println("<h1>Smart Parking System</h1>");
  client.println("<div class='status' id='status'></div>");
  client.println("<button onclick=\"fetch('/book').then(updateStatus)\">Book Slot</button>");
  client.println("<button onclick=\"fetch('/unbook').then(updateStatus)\">Unbook Slot</button>");
  client.println("<script>");
  client.println("function updateStatus() {");
  client.println("  fetch('/status').then(response => response.json()).then(data => {");
  client.println("    const statusDiv = document.getElementById('status');");
  client.println("    if (data.booked) {");
  client.println("      statusDiv.innerText = 'Status: Booked (Reserved)';");
  client.println("      statusDiv.style.color = 'yellow';");
  client.println("    } else if (data.occupied) {");
  client.println("      statusDiv.innerText = 'Status: Occupied (No Parking Available)';");
  client.println("      statusDiv.style.color = 'red';");
  client.println("    } else {");
  client.println("      statusDiv.innerText = 'Status: Vacant (You Can Park)';");
  client.println("      statusDiv.style.color = 'green';");
  client.println("    }");
  client.println("  });");
  client.println("}");
  client.println("updateStatus();");
  client.println("setInterval(updateStatus, 1000);");
  client.println("</script>");
  client.println("</body>");
  client.println("</html>");
}

void updateThingSpeak() {
  int field1 = isOccupied ? 0 : 1;
  int field3 = isBooked ? 1 : 0;

  ThingSpeak.setField(1, field1);
  ThingSpeak.setField(3, field3);

  int responseCode = ThingSpeak.writeFields(channelID, writeAPIKey);
  if (responseCode == 200) {
    Serial.println("Successfully updated ThingSpeak!");
  } else {
    Serial.println("Failed to update ThingSpeak. Response code: " + String(responseCode));
  }
}
