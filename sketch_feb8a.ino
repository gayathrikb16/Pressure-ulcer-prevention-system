#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

// ================= ACCESS POINT SETTINGS =================
const char* apSSID = "PressurePad_AP";
const char* apPassword = "12345678";   // Minimum 8 characters

// ================= FSR SETUP =================
const int fsrPins[5] = {32, 33, 34, 35, 36};

float baseline[5] = {0};
float currentValue[5] = {0};
float normalizedValue[5] = {0};
float loadPercent[5] = {0};
float durationSec[5] = {0};
float riskIndex[5] = {0};

// 🔴 Replace with your calibrated max normalized values
float maxValue[5] = {300, 4000, 4000, 4000, 4000};

// ================= REGION SETTINGS =================
int selectedRegion = 0;  // 0=Heel, 1=Hip, 2=Shoulder
float loadThreshold = 0.25;
float regionFactor = 1.4;

// ================= TIMING =================
const int baselineTime = 5000;
const int sampleInterval = 200;
const int samplesForAverage = 5;

// ===================================================

void setup() {
  Serial.begin(115200);
  delay(2000);

  // Start Access Point
  WiFi.softAP(apSSID, apPassword);

  Serial.println("Access Point Started");
  Serial.print("Connect to WiFi: ");
  Serial.println(apSSID);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Baseline Calibration
  Serial.println("Starting Baseline Calibration...");
  calculateBaseline();
  Serial.println("Baseline Complete.");

  updateRegionSettings();

  // Web routes
  server.on("/", handleRoot);
  server.on("/setRegion", handleSetRegion);

  server.begin();
}

// ===================================================

void loop() {

  server.handleClient();
  readSensors();

  Serial.println("====================================");

  if (selectedRegion == 0) Serial.println("REGION: HEEL");
  if (selectedRegion == 1) Serial.println("REGION: HIP");
  if (selectedRegion == 2) Serial.println("REGION: SHOULDER");

  Serial.println("------------------------------------");

  for (int i = 0; i < 5; i++) {

    normalizedValue[i] = currentValue[i] - baseline[i];
    if (normalizedValue[i] < 0) normalizedValue[i] = 0;

    loadPercent[i] = normalizedValue[i] / maxValue[i];
    if (loadPercent[i] > 1.0) loadPercent[i] = 1.0;

    if (loadPercent[i] > loadThreshold) {
      durationSec[i] += 1;
    } else {
      durationSec[i] -= 1;
      if (durationSec[i] < 0) durationSec[i] = 0;
    }

    riskIndex[i] = loadPercent[i] * durationSec[i] * regionFactor;

    String riskLevel;
    if (riskIndex[i] < 5) riskLevel = "LOW";
    else if (riskIndex[i] < 15) riskLevel = "MEDIUM";
    else riskLevel = "HIGH";

    Serial.print("FSR");
    Serial.print(i + 1);
    Serial.print(" | Load: ");
    Serial.print(loadPercent[i] * 100, 1);
    Serial.print("% | Time: ");
    Serial.print(durationSec[i]);
    Serial.print("s | Th: ");
    Serial.print(loadThreshold * 100, 0);
    Serial.print("% | Risk: ");
    Serial.println(riskLevel);
  }

  Serial.println("====================================\n");

  delay(1000);
}

// ===================================================
// REGION SETTINGS
// ===================================================

void updateRegionSettings() {

  if (selectedRegion == 0) {        // Heel
    loadThreshold = 0.25;
    regionFactor = 1.4;
  }
  else if (selectedRegion == 1) {   // Hip
    loadThreshold = 0.35;
    regionFactor = 1.1;
  }
  else if (selectedRegion == 2) {   // Shoulder
    loadThreshold = 0.30;
    regionFactor = 1.0;
  }
}

// ===================================================
// WEB HANDLERS
// ===================================================

void handleSetRegion() {

  if (server.hasArg("region")) {
    selectedRegion = server.arg("region").toInt();
    updateRegionSettings();
  }

  server.send(200, "text/plain", "Region Updated");
}

void handleRoot() {

  String regionName = "";
  if (selectedRegion == 0) regionName = "Heel";
  if (selectedRegion == 1) regionName = "Hip";
  if (selectedRegion == 2) regionName = "Shoulder";

  String page = "<html><head>";
  page += "<meta http-equiv='refresh' content='2'>";
  page += "<style>";
  page += "body{font-family:Arial;text-align:center;background:#f9f9f9;}";
  page += ".grid{display:grid;grid-template-columns:120px 120px 120px;grid-gap:15px;justify-content:center;margin-top:30px;}";
  page += ".block{width:120px;height:130px;border-radius:15px;display:flex;flex-direction:column;align-items:center;justify-content:center;font-size:12px;font-weight:bold;color:#333;}";
  page += "</style>";
  page += "</head><body>";

  page += "<h2>Pressure Ulcer Risk Monitoring</h2>";

  page += "<h3>Select Body Region</h3>";
  page += "<select onchange='setRegion(this.value)'>";

  page += "<option value='0'";
  if (selectedRegion == 0) page += " selected";
  page += ">Heel</option>";

  page += "<option value='1'";
  if (selectedRegion == 1) page += " selected";
  page += ">Hip</option>";

  page += "<option value='2'";
  if (selectedRegion == 2) page += " selected";
  page += ">Shoulder</option>";

  page += "</select>";

  page += "<h3>Current Region: " + regionName + "</h3>";

  page += "<div class='grid'>";

  // -------- FIRST ROW (FSR1 FSR2 FSR3) --------
  for (int i = 0; i < 3; i++) {

    String riskLevel;
    String color;

    if (riskIndex[i] < 5) {
      riskLevel = "LOW";
      color = "#B8E6B8";
    }
    else if (riskIndex[i] < 15) {
      riskLevel = "MEDIUM";
      color = "#FFF3A3";
    }
    else {
      riskLevel = "HIGH";
      color = "#FFB38A";
    }

    page += "<div class='block' style='background:" + color + ";'>";
    page += "FSR " + String(i + 1) + "<br>";
    page += "Risk: " + riskLevel + "<br>";
    page += "Load: " + String(loadPercent[i] * 100, 1) + "%<br>";
    page += "Time: " + String(durationSec[i]) + "s<br>";
    page += "Th: " + String(loadThreshold * 100, 0) + "%";
    page += "</div>";
  }

  // -------- SECOND ROW (empty, FSR4, empty) --------
  page += "<div></div>";  // empty left

  int i = 3;
  String riskLevel4;
  String color4;

  if (riskIndex[i] < 5) {
    riskLevel4 = "LOW";
    color4 = "#B8E6B8";
  }
  else if (riskIndex[i] < 15) {
    riskLevel4 = "MEDIUM";
    color4 = "#FFF3A3";
  }
  else {
    riskLevel4 = "HIGH";
    color4 = "#FFB38A";
  }

  page += "<div class='block' style='background:" + color4 + ";'>";
  page += "FSR 4<br>";
  page += "Risk: " + riskLevel4 + "<br>";
  page += "Load: " + String(loadPercent[3] * 100, 1) + "%<br>";
  page += "Time: " + String(durationSec[3]) + "s<br>";
  page += "Th: " + String(loadThreshold * 100, 0) + "%";
  page += "</div>";

  page += "<div></div>";  // empty right

  // -------- THIRD ROW (empty, FSR5, empty) --------
  page += "<div></div>";  // empty left

  i = 4;
  String riskLevel5;
  String color5;

  if (riskIndex[i] < 5) {
    riskLevel5 = "LOW";
    color5 = "#B8E6B8";
  }
  else if (riskIndex[i] < 15) {
    riskLevel5 = "MEDIUM";
    color5 = "#FFF3A3";
  }
  else {
    riskLevel5 = "HIGH";
    color5 = "#FFB38A";
  }

  page += "<div class='block' style='background:" + color5 + ";'>";
  page += "FSR 5<br>";
  page += "Risk: " + riskLevel5 + "<br>";
  page += "Load: " + String(loadPercent[4] * 100, 1) + "%<br>";
  page += "Time: " + String(durationSec[4]) + "s<br>";
  page += "Th: " + String(loadThreshold * 100, 0) + "%";
  page += "</div>";

  page += "<div></div>";  // empty right

  page += "</div>";

  page += "<script>";
  page += "function setRegion(region){ fetch('/setRegion?region=' + region); }";
  page += "</script>";

  page += "</body></html>";

  server.send(200, "text/html", page);
}

// ===================================================
// BASELINE FUNCTION
// ===================================================

void calculateBaseline() {

  unsigned long startTime = millis();
  int count = 0;

  while (millis() - startTime < baselineTime) {

    for (int i = 0; i < 5; i++) {
      baseline[i] += analogRead(fsrPins[i]);
    }

    count++;
    delay(50);
  }

  for (int i = 0; i < 5; i++) {
    baseline[i] = baseline[i] / count;
  }
}

// ===================================================
// SENSOR READING
// ===================================================

void readSensors() {

  for (int i = 0; i < 5; i++) {

    long sum = 0;

    for (int j = 0; j < samplesForAverage; j++) {
      sum += analogRead(fsrPins[i]);
      delay(sampleInterval / samplesForAverage);
    }

    currentValue[i] = sum / samplesForAverage;
  }
}
