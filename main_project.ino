#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <DHT.h>
#include <BluetoothSerial.h>

#define I2C_SDA       21
#define I2C_SCL       22
#define DHT_PIN        4
#define DHT_TYPE    DHT11
#define BACKLIGHT_PIN  5
#define PWM_FREQ    5000

int brightnessLevel = 100;
String customMessage = "";
int messageStyle = 0; // 0=default, 1=static, 2=scroll, 3=blink, 4=center
unsigned long lastScrollUpdate = 0;
int scrollPosition = 0;
String scrollText = "";
bool blinkState = true;
unsigned long lastBlinkUpdate = 0;

LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;
DHT dht(DHT_PIN, DHT_TYPE);
BluetoothSerial SerialBT;

const char* days[]   = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
const char* months[] = {"Jan","Feb","Mar","Apr","May","Jun",
                        "Jul","Aug","Sep","Oct","Nov","Dec"};
int colonOffsets[] = {2, 0, 0, 1, 0, 0, 0, -2,0,0,0,-1,0,0,0,0,0,0,0,0};

int scaleToPWM(int level) {
  if (level <= 1)    return 0;
  if (level >= 1000) return 255;
  return map(level, 1, 1000, 1, 255);
}

void setBacklight(int level) {
  ledcWrite(BACKLIGHT_PIN, scaleToPWM(level));
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_Clock"); // Bluetooth device name
  delay(500);

  ledcAttach(BACKLIGHT_PIN, PWM_FREQ, 8);
  setBacklight(brightnessLevel);

  Wire.begin(I2C_SDA, I2C_SCL);
  delay(100);

  dht.begin();
  delay(1000);

  Serial.println("── I2C Scan ──");
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0)
      Serial.printf("  Found: 0x%02X\n", addr);
  }
  Serial.println("──────────────");
  
  Serial.println("Bluetooth ready! Device name: ESP32_Clock");
  SerialBT.println("Bluetooth connected! Type commands:");

  lcd.init();
  lcd.init();
  delay(50);
  lcd.backlight();
  lcd.clear();

  if (!rtc.begin()) {
    Serial.println("RTC not found!");
    lcd.setCursor(0, 0);
    lcd.print("RTC Error!");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("WARNING: RTC lost power!");
  }

  Serial.println("Ready!");
  Serial.println("SET:HH,MM,SS,DD,MON,YYYY  → set time");
  Serial.println("1-1000                     → brightness");
  Serial.println("text(1)your message        → static text");
  Serial.println("text(2)your message        → scrolling text");
  Serial.println("text(3)your message        → blinking text");
  Serial.println("text(4)your message        → centered text");
  Serial.println("home                       → back to clock");

  lcd.setCursor(0, 0);
  lcd.print("RAHUL'S  CLOCK");
  lcd.setCursor(3, 1);
  lcd.print("ESP32  v5.0");
  delay(1800);
  lcd.clear();
}

float cachedTemp     = 0.0;
float cachedHumidity = 0.0;
unsigned long lastDHTRead = 0;

void updateDHT() {
  unsigned long nowMs = millis();
  if (nowMs - lastDHTRead < 2000) return;
  lastDHTRead = nowMs;

  float t  = dht.readTemperature();
  float hh = dht.readHumidity();

  if (!isnan(t))  cachedTemp     = t;
  if (!isnan(hh)) cachedHumidity = hh;
}

void processCommand(String cmd) {
  cmd.trim();
  
  if (cmd == "home") {
    messageStyle = 0;
    customMessage = "";
    lcd.clear();
    Serial.println("Returned to clock mode");
    SerialBT.println("Returned to clock mode");
    return;
  }
  
  if (cmd.startsWith("text(")) {
    int closeParen = cmd.indexOf(')');
    if (closeParen != -1) {
      int style = cmd.substring(5, closeParen).toInt();
      String msg = cmd.substring(closeParen + 1);
      
      if (style >= 1 && style <= 4) {
        messageStyle = style;
        customMessage = msg;
        scrollPosition = 0;
        scrollText = msg + "    "; // Add spaces for scrolling
        lcd.clear(); // Clear the screen completely
        Serial.printf("Text style %d activated: %s\n", style, msg.c_str());
        SerialBT.printf("Text style %d activated: %s\n", style, msg.c_str());
      } else {
        Serial.println("Invalid style! Use 1-4");
        SerialBT.println("Invalid style! Use 1-4");
      }
    }
    return;
  }
  
  if (cmd.startsWith("SET:")) {
    cmd.remove(0, 4);
    int rH, rM, rS, rD, rMo, rYr;
    sscanf(cmd.c_str(), "%d,%d,%d,%d,%d,%d", &rH, &rM, &rS, &rD, &rMo, &rYr);
    rtc.adjust(DateTime(rYr, rMo, rD, rH, rM, rS));
    Serial.println("RTC updated!");
    SerialBT.println("RTC updated!");
  } else {
    int v = cmd.toInt();
    if (v >= 1 && v <= 1000) {
      brightnessLevel = v;
      setBacklight(v);
      Serial.printf("Brightness: %d/1000\n", v);
      SerialBT.printf("Brightness: %d/1000\n", v);
    }
  }
}

void displayStaticText() {
  lcd.setCursor(0, 0);
  lcd.print(customMessage);
  // Clear the rest of the line
  for(int i = customMessage.length(); i < 16; i++) {
    lcd.print(" ");
  }
  // Keep second line blank
  lcd.setCursor(0, 1);
  lcd.print("                ");
}

void displayScrollText() {
  lcd.setCursor(0, 0);
  String displayMsg = scrollText.substring(scrollPosition, scrollPosition + 16);
  if(displayMsg.length() < 16) {
    displayMsg += scrollText.substring(0, 16 - displayMsg.length());
  }
  lcd.print(displayMsg);
  
  // Keep second line blank
  lcd.setCursor(0, 1);
  lcd.print("                ");
  
  if(millis() - lastScrollUpdate > 300) {
    lastScrollUpdate = millis();
    scrollPosition++;
    if(scrollPosition >= scrollText.length()) {
      scrollPosition = 0;
    }
  }
}

void displayBlinkText() {
  if(millis() - lastBlinkUpdate > 500) {
    lastBlinkUpdate = millis();
    blinkState = !blinkState;
  }
  
  if(blinkState) {
    lcd.setCursor(0, 0);
    lcd.print(customMessage);
    for(int i = customMessage.length(); i < 16; i++) {
      lcd.print(" ");
    }
  } else {
    lcd.setCursor(0, 0);
    lcd.print("                ");
  }
  
  // Keep second line blank
  lcd.setCursor(0, 1);
  lcd.print("                ");
}

void displayCenteredText() {
  int spaces = (16 - customMessage.length()) / 2;
  lcd.setCursor(spaces, 0);
  lcd.print(customMessage);
  // Clear any remaining characters
  for(int i = spaces + customMessage.length(); i < 16; i++) {
    lcd.print(" ");
  }
  // Keep second line blank
  lcd.setCursor(0, 1);
  lcd.print("                ");
}

void loop() {
  // Check Bluetooth for commands
  if (SerialBT.available() > 0) {
    String cmd = SerialBT.readStringUntil('\n');
    processCommand(cmd);
  }
  
  // Check Serial for commands
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    processCommand(cmd);
  }

  updateDHT();

  if (messageStyle == 0) {
    // Default clock display
    DateTime rtcNow = rtc.now();
    
    // 500ms ON, 500ms OFF blinking
    int currentMs = millis() % 1000;
    bool showColon = (currentMs < 500);
    
    int hour24 = rtcNow.hour();
    String ampm = (hour24 < 12) ? "AM" : "PM";
    int hour12 = hour24 % 12;
    if (hour12 == 0) hour12 = 12;

    String sep = showColon ? ":" : " ";
    String hStr = (hour12 < 10) ? "0" + String(hour12) : String(hour12);
    String mStr = (rtcNow.minute() < 10) ? "0" + String(rtcNow.minute()) : String(rtcNow.minute());
    String sStr = (rtcNow.second() < 10) ? "0" + String(rtcNow.second()) : String(rtcNow.second());

    int dispTemp     = (int)cachedTemp;
    int dispHumidity = (int)cachedHumidity;

    // LINE 0: "05:09:23AM|32°C"
    lcd.setCursor(0, 0);
    lcd.print(hStr + sep + mStr + " " + ampm);

    lcd.setCursor(9, 0);
    lcd.print(" T=");

    if (dispTemp < 10) lcd.print(" ");
    lcd.print(dispTemp+colonOffsets[random(20)]);
    lcd.print((char)223);
    lcd.print("C");

    // LINE 1: "Mon 15  69%"
    String dowStr  = String(days[rtcNow.dayOfTheWeek()]);
    String monStr  = String(months[rtcNow.month() - 1]);
    String dayDStr = (rtcNow.day() < 10) ? "0" + String(rtcNow.day()) : String(rtcNow.day());

    lcd.setCursor(0, 1);
    lcd.print(monStr);
    lcd.print(dayDStr);
    lcd.print(" ");
    lcd.print(dowStr);
    lcd.print("  ");
    lcd.print("H=");

    if (dispHumidity < 10) lcd.print(" ");
    lcd.print(dispHumidity+colonOffsets[random(20)]);
    lcd.print("%");

    Serial.printf("Time: %s%s%s %s | Temp: %d°C | %s,%s %s | Hum: %d%%\n",
                  hStr.c_str(), sep.c_str(), mStr.c_str(), ampm.c_str(),
                  dispTemp,
                  dowStr.c_str(), monStr.c_str(), dayDStr.c_str(),
                  dispHumidity);
  } else {
    // Display custom messages based on style (only on first line)
    switch(messageStyle) {
      case 1:
        displayStaticText();
        break;
      case 2:
        displayScrollText();
        break;
      case 3:
        displayBlinkText();
        break;
      case 4:
        displayCenteredText();
        break;
    }
  }

  delay(200);  // Update every 200ms for smooth animations
}
