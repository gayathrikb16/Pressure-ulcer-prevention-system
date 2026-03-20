/*
Adaptive Region-Aware Pressure Ulcer Prevention System
Author: Gayathri K B

Description:
Real-time pressure monitoring using FSR sensors with automated
air-cell actuation based on a weighted risk model.
*/

#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

const char* apSSID = "PressurePad_AP";
const char* apPassword = "12345678";

const int fsrPins[5] = {32, 33, 34, 35, 36};

#define FSR_TOP     0
#define FSR_RIGHT   1
#define FSR_CENTER  2
#define FSR_LEFT    3
#define FSR_BOTTOM  4

const int airCellPins[5] = {18, 19, 21, 22, 23};

float baseline[5] = {0};
float currentValue[5] = {0};
float normalizedValue[5] = {0};
float loadPercent[5] = {0};
float durationSec[5] = {0};
float riskIndex[5] = {0};

//Maximum calibrated values
float maxValue[5] = {300, 4000, 5000, 4000, 4000};

//CLINICAL WEIGHTING
float sensorWeight[5] = {
  1.0,   // TOP
  1.0,   // RIGHT
  1.6,   // CENTER (higher weight - bony prominence)
  1.0,   // LEFT
  1.0    // BOTTOM
};

//REGION SETTINGS
int selectedRegion = 0;
float loadThreshold = 0.25;
float regionFactor = 1.4;

const int baselineTime = 5000;
const int sampleInterval = 200;
const int samplesForAverage = 5;

void setup() {

  Serial.begin(115200);
  delay(2000);

  WiFi.softAP(apSSID, apPassword);

  Serial.println("Access Point Started");
  Serial.println(WiFi.softAPIP());

  // Configure air cell outputs
  for (int i = 0; i < 5; i++) {
    pinMode(airCellPins[i], OUTPUT);
    digitalWrite(airCellPins[i], LOW);
  }

  calculateBaseline();
  updateRegionSettings();

  server.on("/", handleRoot);
  server.on("/setRegion", handleSetRegion);
  server.begin();
}

void loop() {

  server.handleClient();
  readSensors();

  Serial.println("=========== CROSS STRUCTURE ==========");

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

    //Weighted Risk Formula
    riskIndex[i] = sensorWeight[i] *
                   loadPercent[i] *
                   durationSec[i] *
                   regionFactor;

    String riskLevel;
    if (riskIndex[i] < 5) riskLevel = "LOW";
    else if (riskIndex[i] < 15) riskLevel = "MEDIUM";
    else riskLevel = "HIGH";

    Serial.print("FSR ");
    Serial.print(i);
    Serial.print(" | Load: ");
    Serial.print(loadPercent[i] * 100, 1);
    Serial.print("% | Time: ");
    Serial.print(durationSec[i]);
    Serial.print("s | Risk: ");
    Serial.println(riskLevel);

    //Automatic Air Cell Actuation
    if (riskIndex[i] >= 15) {

      digitalWrite(airCellPins[i], HIGH);

      Serial.print(">>> ACTUATION: Air Cell ");
      Serial.print(i);
      Serial.println(" ACTIVATED (HIGH RISK)");
    }
    else {
      digitalWrite(airCellPins[i], LOW);
    }
  }

  if (riskIndex[FSR_CENTER] >= 15) {

    Serial.println(">>> CENTRAL PEAK PRESSURE DETECTED");
    Serial.println(">>> FULL OFFLOADING MODE ACTIVATED");

    digitalWrite(airCellPins[FSR_TOP], HIGH);
    digitalWrite(airCellPins[FSR_BOTTOM], HIGH);
    digitalWrite(airCellPins[FSR_LEFT], HIGH);
    digitalWrite(airCellPins[FSR_RIGHT], HIGH);
  }

  Serial.println("======================================\n");
  delay(1000);
}

//REGION SETTINGS

void updateRegionSettings() {

  if (selectedRegion == 0) {        //Heel
    loadThreshold = 0.25;
    regionFactor = 1.4;
  }
  else if (selectedRegion == 1) {   //Hip
    loadThreshold = 0.35;
    regionFactor = 1.1;
  }
  else if (selectedRegion == 2) {   //Shoulder
    loadThreshold = 0.30;
    regionFactor = 1.0;
  }
}

// WEB HANDLERS

void handleSetRegion() {

  if (server.hasArg("region")) {
    selectedRegion = server.arg("region").toInt();
    updateRegionSettings();
  }

  server.send(200, "text/plain", "Region Updated");
}

String getRiskLevel(int i, String &color) {

  if (riskIndex[i] < 5) {
    color = "#B8E6B8";
    return "LOW";
  }
  else if (riskIndex[i] < 15) {
    color = "#FFF3A3";
    return "MEDIUM";
  }
  else {
    color = "#FFB38A";
    return "HIGH";
  }
}

void addBlock(String &page, int i) {

  String color;
  String risk = getRiskLevel(i, color);

  page += "<div class='block' style='background:" + color + ";'>";
  page += "FSR " + String(i) + "<br>";
  page += "Risk: " + risk + "<br>";
  page += "Load: " + String(loadPercent[i] * 100, 1) + "%<br>";
  page += "Time: " + String(durationSec[i]) + "s";
  page += "</div>";
}

void handleRoot() {

  String regionName = "";
  if (selectedRegion == 0) regionName = "Heel";
  if (selectedRegion == 1) regionName = "Hip";
  if (selectedRegion == 2) regionName = "Shoulder";

  String page = "<html><head>";
  page += "<meta http-equiv='refresh' content='2'>";
  page += "<style>";
  page += "body{font-family:Arial;text-align:center;background:#f4f4f4;}";
  page += ".grid{display:grid;grid-template-columns:120px 120px 120px;grid-gap:15px;justify-content:center;margin-top:30px;}";
  page += ".block{width:120px;height:130px;border-radius:15px;display:flex;flex-direction:column;align-items:center;justify-content:center;font-size:12px;font-weight:bold;}";
  page += "select{padding:8px;margin-top:10px;font-size:14px;}";
  page += "</style>";
  page += "</head><body>";

  page += "<h2>Pressure Ulcer Risk - CROSS Layout</h2>";

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

  page += "<h4>Current Region: " + regionName + "</h4>";
  page += "<h4>Region Factor: " + String(regionFactor, 2) + "</h4>";

  page += "<div class='grid'>";

  page += "<div></div>";
  addBlock(page, FSR_TOP);
  page += "<div></div>";

  addBlock(page, FSR_LEFT);
  addBlock(page, FSR_CENTER);
  addBlock(page, FSR_RIGHT);

  page += "<div></div>";
  addBlock(page, FSR_BOTTOM);
  page += "<div></div>";

  page += "</div>";

  page += "<script>";
  page += "function setRegion(region){ fetch('/setRegion?region=' + region); }";
  page += "</script>";

  page += "</body></html>";

  server.send(200, "text/html", page);
}

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
    baseline[i] /= count;
  }
}

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
