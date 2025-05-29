#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <time.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_420_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>

// E-ink Display Configuration (adjust based on your display)
#define EPD_CS 5
#define EPD_DC 15
#define EPD_RST 2
#define EPD_BUSY 4

// Initialize display for 4.2" Waveshare e-paper
GxEPD2_BW<GxEPD2_420_BW, GxEPD2_420_BW::HEIGHT> display(GxEPD2_420_BW(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

// WiFi Configuration
WebServer server(80);
String ssid = "";
String password = "";
bool wifiConfigured = false;
const char* apSSID = "QuoteDisplay_Setup";
const char* apPassword = "12345678";

// Quote Management
const int MAX_QUOTES = 100;
String quotes[MAX_QUOTES];
int quoteCount = 0;
int currentQuoteIndex = 0;
unsigned long lastQuoteChange = 0;
const unsigned long QUOTE_INTERVAL = 60000; // 1 minute

// EEPROM Addresses
const int EEPROM_SIZE = 4096;
const int WIFI_CONFIGURED_ADDR = 0;
const int SSID_ADDR = 10;
const int PASSWORD_ADDR = 50;
const int QUOTE_COUNT_ADDR = 100;
const int QUOTES_START_ADDR = 110;

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  
  // Initialize display
  display.init(115200);
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  
  // Show startup message
  displayMessage("Quote Display Starting...");
  
  // Load configuration from EEPROM
  loadConfiguration();
  
  // Check if WiFi is configured
  if (wifiConfigured && ssid.length() > 0) {
    connectToWiFi();
    if (WiFi.status() == WL_CONNECTED) {
      startWebServer();
      displayMessage("Connected to WiFi\nWeb interface active");
      delay(2000);
    } else {
      startConfigMode();
    }
  } else {
    startConfigMode();
  }
  
  // Load default quotes if none exist
  if (quoteCount == 0) {
    loadDefaultQuotes();
  }
  
  // Display first quote
  displayCurrentQuote();
}

void loop() {
  server.handleClient();
  
  // Check if it's time to change quotes
  if (millis() - lastQuoteChange >= QUOTE_INTERVAL) {
    nextQuote();
    displayCurrentQuote();
    lastQuoteChange = millis();
  }
  
  delay(1000);
}

void loadConfiguration() {
  wifiConfigured = EEPROM.read(WIFI_CONFIGURED_ADDR);
  
  if (wifiConfigured) {
    // Load SSID
    for (int i = 0; i < 32; i++) {
      char c = EEPROM.read(SSID_ADDR + i);
      if (c == '\0') break;
      ssid += c;
    }
    
    // Load Password
    for (int i = 0; i < 32; i++) {
      char c = EEPROM.read(PASSWORD_ADDR + i);
      if (c == '\0') break;
      password += c;
    }
    
    // Load quotes
    quoteCount = EEPROM.read(QUOTE_COUNT_ADDR);
    if (quoteCount > MAX_QUOTES) quoteCount = 0;
    
    // Load each quote (simplified - in reality you'd need better string handling)
    // This is a basic implementation - you might want to use a more robust storage method
  }
}

void saveConfiguration() {
  EEPROM.write(WIFI_CONFIGURED_ADDR, 1);
  
  // Save SSID
  for (int i = 0; i < ssid.length() && i < 31; i++) {
    EEPROM.write(SSID_ADDR + i, ssid[i]);
  }
  EEPROM.write(SSID_ADDR + ssid.length(), '\0');
  
  // Save Password
  for (int i = 0; i < password.length() && i < 31; i++) {
    EEPROM.write(PASSWORD_ADDR + i, password[i]);
  }
  EEPROM.write(PASSWORD_ADDR + password.length(), '\0');
  
  // Save quote count
  EEPROM.write(QUOTE_COUNT_ADDR, quoteCount);
  
  EEPROM.commit();
}

void startConfigMode() {
  displayMessage("Configuration Mode\nConnect to:\n" + String(apSSID));
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID, apPassword);
  
  server.on("/", handleConfigPage);
  server.on("/save", handleSaveConfig);
  server.begin();
  
  Serial.println("Configuration mode started");
  Serial.println("Connect to: " + String(apSSID));
  Serial.println("Password: " + String(apPassword));
}

void connectToWiFi() {
  displayMessage("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.println("IP address: " + WiFi.localIP().toString());
  }
}

void startWebServer() {
  server.on("/", handleMainPage);
  server.on("/quotes", HTTP_GET, handleGetQuotes);
  server.on("/quotes", HTTP_POST, handleAddQuote);
  server.on("/quotes/delete", HTTP_POST, handleDeleteQuote);
  server.on("/quotes/clear", HTTP_POST, handleClearQuotes);
  server.begin();
  
  Serial.println("Web server started on: " + WiFi.localIP().toString());
}

void handleConfigPage() {
  String html = "<!DOCTYPE HTML><html><head><title>Quote Display Setup</title></head><body>";
  html += "<h1>Quote Display WiFi Setup</h1>";
  html += "<form action='/save' method='post'>";
  html += "WiFi SSID: <input type='text' name='ssid' required><br><br>";
  html += "WiFi Password: <input type='password' name='password'><br><br>";
  html += "<input type='submit' value='Save and Connect'>";
  html += "</form></body></html>";
  
  server.send(200, "text/html", html);
}

void handleSaveConfig() {
  ssid = server.arg("ssid");
  password = server.arg("password");
  
  saveConfiguration();
  
  String html = "<!DOCTYPE HTML><html><head><title>Saved</title></head><body>";
  html += "<h1>Configuration Saved!</h1>";
  html += "<p>Device will restart and connect to WiFi...</p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
  delay(2000);
  ESP.restart();
}

void handleMainPage() {
  String html = "<!DOCTYPE HTML><html><head><title>Quote Display Manager</title>";
  html += "<style>body{font-family:Arial;margin:40px;} .quote{border:1px solid #ccc;padding:10px;margin:10px 0;}</style>";
  html += "</head><body>";
  html += "<h1>Quote Display Manager</h1>";
  html += "<h2>Current Quote</h2>";
  html += "<div class='quote'>" + quotes[currentQuoteIndex] + "</div>";
  html += "<h2>Add New Quote</h2>";
  html += "<form action='/quotes' method='post'>";
  html += "<textarea name='quote' rows='4' cols='50' placeholder='Enter your quote here...' required></textarea><br><br>";
  html += "<input type='submit' value='Add Quote'>";
  html += "</form>";
  html += "<h2>Manage Quotes</h2>";
  html += "<button onclick=\"if(confirm('Clear all quotes?')) fetch('/quotes/clear', {method:'POST'}).then(()=>location.reload())\">Clear All Quotes</button>";
  html += "<h2>All Quotes (" + String(quoteCount) + ")</h2>";
  
  for (int i = 0; i < quoteCount; i++) {
    html += "<div class='quote'>" + quotes[i] + " ";
    html += "<button onclick=\"if(confirm('Delete this quote?')) fetch('/quotes/delete', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:'index=" + String(i) + "'}).then(()=>location.reload())\">Delete</button>";
    html += "</div>";
  }
  
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleGetQuotes() {
  DynamicJsonDocument doc(4096);
  JsonArray quotesArray = doc.createNestedArray("quotes");
  
  for (int i = 0; i < quoteCount; i++) {
    quotesArray.add(quotes[i]);
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleAddQuote() {
  String newQuote = server.arg("quote");
  if (newQuote.length() > 0 && quoteCount < MAX_QUOTES) {
    quotes[quoteCount] = newQuote;
    quoteCount++;
    saveConfiguration(); // You'd need to implement quote saving in EEPROM
  }
  
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDeleteQuote() {
  int index = server.arg("index").toInt();
  if (index >= 0 && index < quoteCount) {
    // Shift quotes down
    for (int i = index; i < quoteCount - 1; i++) {
      quotes[i] = quotes[i + 1];
    }
    quoteCount--;
    if (currentQuoteIndex >= quoteCount) currentQuoteIndex = 0;
    saveConfiguration();
  }
  
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleClearQuotes() {
  quoteCount = 0;
  currentQuoteIndex = 0;
  loadDefaultQuotes(); // Reload defaults
  saveConfiguration();
  
  server.sendHeader("Location", "/");
  server.send(303);
}

void loadDefaultQuotes() {
  quotes[0] = "The only way to do great work is to love what you do. - Steve Jobs";
  quotes[1] = "Innovation distinguishes between a leader and a follower. - Steve Jobs";
  quotes[2] = "Life is what happens to you while you're busy making other plans. - John Lennon";
  quotes[3] = "The future belongs to those who believe in the beauty of their dreams. - Eleanor Roosevelt";
  quotes[4] = "It is during our darkest moments that we must focus to see the light. - Aristotle";
  quoteCount = 5;
  currentQuoteIndex = 0;
}

void nextQuote() {
  if (quoteCount > 0) {
    currentQuoteIndex = (currentQuoteIndex + 1) % quoteCount;
  }
}

void displayCurrentQuote() {
  if (quoteCount > 0) {
    displayMessage(quotes[currentQuoteIndex]);
  } else {
    displayMessage("No quotes available");
  }
}

void displayMessage(String message) {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(10, 30);
    
    // Word wrap the message
    String words[100];
    int wordCount = 0;
    int startIndex = 0;
    
    // Simple word splitting
    for (int i = 0; i <= message.length(); i++) {
      if (i == message.length() || message[i] == ' ' || message[i] == '\n') {
        if (i > startIndex) {
          words[wordCount++] = message.substring(startIndex, i);
        }
        startIndex = i + 1;
      }
    }
    
    // Display words with wrapping
    int x = 10, y = 30;
    int lineHeight = 20;
    int maxWidth = display.width() - 20;
    
    for (int i = 0; i < wordCount; i++) {
      int wordWidth = words[i].length() * 12; // Approximate character width
      
      if (x + wordWidth > maxWidth && x > 10) {
        x = 10;
        y += lineHeight;
      }
      
      display.setCursor(x, y);
      display.print(words[i] + " ");
      x += wordWidth + 12; // Space width
    }
  } while (display.nextPage());
}