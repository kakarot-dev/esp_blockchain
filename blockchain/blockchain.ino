#include "FS.h"
#include "LittleFS.h"
#include "mbedtls/sha256.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

#define MAX_TRANSACTIONS 100  // Max chat messages per block
#define MESSAGE_SIZE 500
#define BLOCKCHAIN_DIR "/blocks/"

const char *ssid = "Access Point";
const char *password = "12345678";

DNSServer dnsServer;

AsyncWebServer server(80);

// Chat Message (Transaction)
struct Transaction {
  String sender;
  String message;
  String timestamp;
  String group;

  Transaction() {}
  Transaction(String s, String m, String t, String g) {
    sender = s;
    message = m.substring(0, MESSAGE_SIZE);
    timestamp = t;
    group = g;
  }
};

// Blockchain Block
struct Block {
  int index;
  String timestamp;
  String previousHash;
  String hash;
  Transaction transactions[MAX_TRANSACTIONS];
  int transactionCount;

  Block(int idx, String prevHash) {
    index = idx;
    timestamp = String(millis());
    previousHash = prevHash;
    transactionCount = 0;
    hash = calculateHash();
  }

  bool addTransaction(String sender, String message, String group, String timestamp) {
    if (transactionCount < MAX_TRANSACTIONS) {
      transactions[transactionCount] = Transaction(sender, message, timestamp, group);
      transactionCount++;
      hash = calculateHash();
      return true;
    }
    return false;
  }


  String calculateHash() {
    String data = String(index) + timestamp + previousHash;
    for (int i = 0; i < transactionCount; i++) {
      data += transactions[i].sender + transactions[i].message + transactions[i].timestamp + transactions[i].group;
    }

    uint8_t hashResult[32];
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, (const unsigned char *)data.c_str(), data.length());
    mbedtls_sha256_finish(&ctx, hashResult);
    mbedtls_sha256_free(&ctx);

    String hashString = "";
    for (int i = 0; i < 32; i++) {
      hashString += String(hashResult[i], HEX);
    }
    return hashString;
  }
};

void saveBlockToFS(Block *block) {
  String filePath = String(BLOCKCHAIN_DIR) + "block_" + String(block->index - 1) + ".dat";
  File file = LittleFS.open(filePath, "w");
  Serial.println(block->index);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  file.println("Block " + String(block->index));
  file.println("Hash: " + block->hash);
  file.println("PrevHash: " + block->previousHash);
  file.println("Timestamp: " + block->timestamp);
  for (int j = 0; j < block->transactionCount; j++) {
    file.println("[" + block->transactions[j].timestamp + "] (" + block->transactions[j].group + ") " + block->transactions[j].sender + ": " + block->transactions[j].message);
  }
  file.println("-----------------");
  file.close();
}

int getLastBlockIndexFromFS() {
  int lastIndex = 0;
  File root = LittleFS.open(BLOCKCHAIN_DIR);
  if (!root || !root.isDirectory()) {
    Serial.println("No blockchain directory found, starting from index 0.");
    return 0;
  }
  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();
    file.close();
    if (fileName.startsWith("block_")) {
      int index = fileName.substring(6, fileName.lastIndexOf(".dat")).toInt();
      if (index >= lastIndex) lastIndex = index + 1;
    }
    file = root.openNextFile();
  }
  return lastIndex;
}

String getLastBlockHashFromFS(int index) {
  if (index == 0) return "0";
  String filePath = String(BLOCKCHAIN_DIR) + "block_" + String(index - 1) + ".dat";
  File file = LittleFS.open(filePath, "r");
  if (!file) {
    Serial.println("Could not open last block hash.. Assuming Genesis");
    return "0";
  }
  String lastHash = "0";
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.startsWith("Hash: ")) {
      lastHash = line.substring(6);
      break;
    }
  }
  file.close();
  Serial.println(lastHash);
  return lastHash;
}

Block *currentBlock;
void addBlock() {
  String prevHash = currentBlock ? currentBlock->hash : "0";
  saveBlockToFS(currentBlock);
  delete currentBlock;
  currentBlock = new Block(currentBlock->index + 1, prevHash);
}

void addChatTransaction(String sender, String message, String group, String timestamp) {
  if (!currentBlock->addTransaction(sender, message, group, timestamp)) {
    addBlock();
    currentBlock->addTransaction(sender, message, group, timestamp);
  }
}


void clearBlockchain() {
  File root = LittleFS.open(BLOCKCHAIN_DIR);
  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();
    file.close();
    LittleFS.remove(String(BLOCKCHAIN_DIR) + fileName);
    Serial.println("Deleted " + fileName);
    file = root.openNextFile();
  }
  Serial.println("Cleared Blockchain");
}

void handleNotFound(AsyncWebServerRequest *request) {
  request->redirect("http://192.168.4.1/");
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  esp_log_level_set("*", ESP_LOG_DEBUG);


  Serial.println("Starting ESP32 Blockchain Storage...");

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Initialization Failed!");
    return;
  }

  int lastIndex = getLastBlockIndexFromFS();
  String lastHash = getLastBlockHashFromFS(lastIndex);
  currentBlock = new Block(lastIndex + 1, lastHash);
  Serial.println(lastIndex);

  WiFi.softAP(ssid, password);
  WiFi.setHostname("ESP32_Chat");
  Serial.println("Starting Webserver");
  dnsServer.start(53, "*", WiFi.softAPIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", R"rawliteral(
          <!DOCTYPE html>
          <html>
          <head>
              <title>ESP32 Chat</title>
              <style>
                  body {
                      font-family: sans-serif;
                      text-align: center;
                      display: flex;
                      flex-direction: column;
                      justify-content: center;
                      align-items: center;
                      height: 100vh;
                      margin: 0;
                      background-color: #f4f4f4;
                  }
                  h1 {
                      margin-bottom: 10px;
                  }
                  p {
                      font-size: 18px;
                      margin-bottom: 20px;
                  }
                  .button {
                      background-color: #007bff;
                      color: white;
                      border: none;
                      padding: 12px 24px;
                      font-size: 16px;
                      border-radius: 8px;
                      cursor: pointer;
                      transition: 0.3s;
                  }
                  .button:hover {
                      background-color: #0056b3;
                  }
              </style>
          </head>
          <body>
              <h1>Welcome to ESP32 Chat!</h1>
              <p>Here is your identifier:</p>
              <p id="identifier"><strong>Loading...</strong></p>
              <button class="button" onclick="openInBrowser()">Open in Browser</button>

              <script>
                  function generateIdentifier() {
                      let id = localStorage.getItem("deviceIdentifier");
                      if (!id) {
                          id = btoa(navigator.userAgent).substring(0, 15); 
                          localStorage.setItem("deviceIdentifier", id);
                      }
                      return id;
                  }

                function openInBrowser() {
                  var url = "http://192.168.4.1/chat";

                  if (navigator.userAgent.toLowerCase().indexOf('android') > -1) {
                      window.location.href = "intent://" + url.replace("http://", "") +
                          "/#Intent;scheme=http;package=com.android.chrome;end;";
                  } else if (navigator.userAgent.toLowerCase().indexOf('iphone') > -1 || 
                            navigator.userAgent.toLowerCase().indexOf('ipad') > -1) {
                      setTimeout(function() {
                          window.location.href = "http://192.168.4.1/chat";
                      }, 500); // Delay to allow portal exit
                  } else {
                      window.location.href = url; // Normal case
                  }
                }

                  document.getElementById("identifier").innerText = generateIdentifier();
              </script>
          </body>
          </html>
      )rawliteral");
  });
  server.on("/api/messages", HTTP_GET, [](AsyncWebServerRequest *request) {
    String group = "General";  // Default group
    if (request->hasParam("group")) {
      group = request->getParam("group")->value();
    }

    String response = "[";
    for (int i = 0; i < currentBlock->transactionCount; i++) {
      Transaction t = currentBlock->transactions[i];
      if (t.group == group) {
        response += "{";
        response += "\"sender\":\"" + t.sender + "\",";
        response += "\"message\":\"" + t.message + "\",";
        response += "\"timestamp\":\"" + t.timestamp + "\"";
        response += "},";
      }
    }
    if (response.length() > 1) response.remove(response.length() - 1);  // Remove last comma
    response += "]";

    request->send(200, "application/json", response);
  });

  server.on("/api/messages", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("sender", true) && request->hasParam("message", true) && request->hasParam("group", true) && request->hasParam("timestamp", true)) {

      String sender = request->getParam("sender", true)->value();
      String message = request->getParam("message", true)->value();
      String group = request->getParam("group", true)->value();
      String timestamp = request->getParam("timestamp", true)->value();  // Use UI timestamp

      addChatTransaction(sender, message, group, timestamp);
      request->send(200, "application/json", "{\"status\": \"ok\"}");
    } else {
      request->send(400, "application/json", "{\"error\": \"Missing parameters\"}");
    }
  });

  // Serve index.html when /chat is accessed
  server.on("/chat", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  // Serve all static files (JS, CSS, images, etc.) from React's build
  server.serveStatic("/static", LittleFS, "/static");
  server.serveStatic("/assets", LittleFS, "/assets");


  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Web Server Started at: " + String(WiFi.softAPIP()));
}

void loop() {
  dnsServer.processNextRequest();
}
