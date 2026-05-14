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

int brightnessLevel = 500;
String customMessage = "";
String customMessageLine2 = "";
int messageStyle = 0; // 0=default, 1-6=text styles
unsigned long lastAnimationUpdate = 0;
int animationPosition = 0;
String scrollText = "";
bool blinkState = true;
unsigned long lastBlinkUpdate = 0;
int animationSpeed = 50;
int textSpeed = 50;
bool systemOn = true;
int cursorPos = 0;
int cursorPos2 = 0;
int charCount = 0;
int charCount2 = 0;
unsigned long lastCharTime = 0;
unsigned long lastCharTime2 = 0;
bool animationCompleted = false;
bool line1Completed = false;
bool line2Completed = false;
int animationStage = 0; // 0=line1, 1=line2, 2=completed

LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;
DHT dht(DHT_PIN, DHT_TYPE);
BluetoothSerial SerialBT;

const char* days[]   = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
const char* months[] = {"Jan","Feb","Mar","Apr","May","Jun",
                        "Jul","Aug","Sep","Oct","Nov","Dec"};

int scaleToPWM(int level) {
  if (level <= 1)    return 0;
  if (level >= 1000) return 255;
  return map(level, 1, 1000, 1, 255);
}

void setBacklight(int level) {
  if (!systemOn) level = 0;
  ledcWrite(BACKLIGHT_PIN, scaleToPWM(level));
}

void splitMessage(String msg) {
  int atPos = msg.indexOf('@');
  if (atPos != -1) {
    customMessage = msg.substring(0, atPos);
    customMessageLine2 = msg.substring(atPos + 1);
    if (customMessage.length() > 16) customMessage = customMessage.substring(0, 16);
    if (customMessageLine2.length() > 16) customMessageLine2 = customMessageLine2.substring(0, 16);
  } else {
    customMessage = msg;
    customMessageLine2 = "";
    if (customMessage.length() > 16) customMessage = customMessage.substring(0, 16);
  }
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_Clock");
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
  SerialBT.println("=== ESP32 Clock Commands ===");

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

  Serial.println("\n=== COMMANDS ===");
  Serial.println("SET:HH,MM,SS,DD,MON,YYYY  → Set time");
  Serial.println("bright(0-1000)            → Set brightness");
  Serial.println("speed(1-100)              → Set animation speed (100=fastest)");
  Serial.println("offme                      → Turn OFF display & system");
  Serial.println("onme                       → Turn ON display & system");
  Serial.println("\n--- TEXT STYLES ---");
  Serial.println("text(1)Your message       → Static Left Aligned (supports @)");
  Serial.println("text(2)Your message       → Center Aligned (supports @)");
  Serial.println("text(3)Your message       → Scrolling Left to Right (NO @ support)");
  Serial.println("text(4)Your message       → Blinking Text (supports @)");
  Serial.println("text(5)Your message       → Wipe Cursor Effect (runs once, sequential, centered)");
  Serial.println("text(6)Your message       → Typewriter Effect (runs once, sequential, centered)");
  Serial.println("\nUse @ for two-line display (e.g., Rahul@Manna)");
  Serial.println("Just type any text (e.g., RAHUL) → Centered static text (Style 2)");
  Serial.println("home                       → Back to clock\n");

  lcd.setCursor(0, 0);
  lcd.print("RAHUL'S  CLOCK");
  lcd.setCursor(3, 1);
  lcd.print("ESP32  v12.0");
  delay(1800);
  lcd.clear();
}

float cachedTemp = 0.0;
float cachedHumidity = 0.0;
unsigned long lastDHTRead = 0;

void updateDHT() {
  unsigned long nowMs = millis();
  if (nowMs - lastDHTRead < 2000) return;
  lastDHTRead = nowMs;

  float t = dht.readTemperature();
  float hh = dht.readHumidity();

  if (!isnan(t)) cachedTemp = t;
  if (!isnan(hh)) cachedHumidity = hh;
}

void processCommand(String cmd) {
  cmd.trim();
  
  if (cmd == "home") {
    messageStyle = 0;
    customMessage = "";
    customMessageLine2 = "";
    systemOn = true;
    setBacklight(brightnessLevel);
    lcd.clear();
    lcd.backlight();
    Serial.println("→ Clock mode");
    SerialBT.println("→ Clock mode");
    return;
  }
  
  if (cmd == "offme") {
    systemOn = false;
    setBacklight(0);
    lcd.noBacklight();
    lcd.clear();
    lcd.display();
    Serial.println("→ System OFF");
    SerialBT.println("→ System OFF");
    return;
  }
  
  if (cmd == "onme") {
    systemOn = true;
    setBacklight(brightnessLevel);
    lcd.backlight();
    lcd.clear();
    messageStyle = 0;
    Serial.println("→ System ON");
    SerialBT.println("→ System ON");
    return;
  }
  
  if (cmd.startsWith("bright(")) {
    int closeParen = cmd.indexOf(')');
    if (closeParen != -1) {
      int bright = cmd.substring(7, closeParen).toInt();
      if (bright >= 0 && bright <= 1000) {
        brightnessLevel = bright;
        if (systemOn) setBacklight(bright);
        Serial.printf("→ Brightness: %d/1000\n", bright);
        SerialBT.printf("→ Brightness: %d/1000\n", bright);
      }
    }
    return;
  }
  
  if (cmd.startsWith("speed(")) {
    int closeParen = cmd.indexOf(')');
    if (closeParen != -1) {
      int speed = cmd.substring(6, closeParen).toInt();
      if (speed >= 1 && speed <= 100) {
        animationSpeed = speed;
        textSpeed = map(speed, 1, 100, 300, 30);
        Serial.printf("→ Speed: %d/100 (delay: %dms)\n", speed, textSpeed);
        SerialBT.printf("→ Speed: %d/100\n", speed);
      }
    }
    return;
  }
  
  if (cmd.startsWith("text(")) {
    int closeParen = cmd.indexOf(')');
    if (closeParen != -1) {
      int style = cmd.substring(5, closeParen).toInt();
      String msg = cmd.substring(closeParen + 1);
      
      if (style >= 1 && style <= 6) {
        messageStyle = style;
        animationPosition = 0;
        cursorPos = 0;
        cursorPos2 = 0;
        charCount = 0;
        charCount2 = 0;
        animationCompleted = false;
        line1Completed = false;
        line2Completed = false;
        animationStage = 0;
        
        // Split message at @ symbol
        splitMessage(msg);
        
        // For text(3) - ignore @ symbol
        if (style == 3) {
          customMessage = msg;
          customMessageLine2 = "";
          if (customMessage.indexOf('@') != -1) {
            customMessage.replace("@", "");
          }
        }
        
        scrollText = customMessage + "    ";
        
        lcd.clear();
        Serial.printf("→ Style %d: %s", style, customMessage.c_str());
        if (customMessageLine2.length() > 0 && style != 3) {
          Serial.printf(" [%s]", customMessageLine2.c_str());
        }
        Serial.println();
        SerialBT.printf("→ Style %d activated\n", style);
      }
    }
    return;
  }
  
  // Normal text command with @ support
  if (!cmd.startsWith("SET:") && cmd.length() > 0 && cmd != "home" && 
      !cmd.startsWith("text(") && !cmd.startsWith("bright(") && 
      !cmd.startsWith("speed(") && cmd != "offme" && cmd != "onme") {
    messageStyle = 2;
    splitMessage(cmd);
    lcd.clear();
    Serial.printf("→ Centered: %s", customMessage.c_str());
    if (customMessageLine2.length() > 0) {
      Serial.printf(" [%s]", customMessageLine2.c_str());
    }
    Serial.println();
    SerialBT.printf("→ Centered: %s", customMessage.c_str());
    if (customMessageLine2.length() > 0) {
      SerialBT.printf(" [%s]", customMessageLine2.c_str());
    }
    SerialBT.println();
    return;
  }
  
  if (cmd.startsWith("SET:")) {
    cmd.remove(0, 4);
    int rH, rM, rS, rD, rMo, rYr;
    sscanf(cmd.c_str(), "%d,%d,%d,%d,%d,%d", &rH, &rM, &rS, &rD, &rMo, &rYr);
    rtc.adjust(DateTime(rYr, rMo, rD, rH, rM, rS));
    Serial.println("→ RTC updated!");
    SerialBT.println("→ RTC updated!");
  }
}

// Style 1: Static Left Aligned (supports @)
void displayStyle1() {
  lcd.setCursor(0, 0);
  lcd.print(customMessage);
  for(int i = customMessage.length(); i < 16; i++) lcd.print(" ");
  
  if (customMessageLine2.length() > 0) {
    lcd.setCursor(0, 1);
    lcd.print(customMessageLine2);
    for(int i = customMessageLine2.length(); i < 16; i++) lcd.print(" ");
  } else {
    lcd.setCursor(0, 1);
    lcd.print("                ");
  }
}

// Style 2: Center Aligned (supports @)
void displayStyle2() {
  int spaces1 = (16 - customMessage.length()) / 2;
  if(spaces1 < 0) spaces1 = 0;
  lcd.setCursor(spaces1, 0);
  lcd.print(customMessage);
  for(int i = customMessage.length(); i < 16; i++) lcd.print(" ");
  
  if (customMessageLine2.length() > 0) {
    int spaces2 = (16 - customMessageLine2.length()) / 2;
    if(spaces2 < 0) spaces2 = 0;
    lcd.setCursor(spaces2, 1);
    lcd.print(customMessageLine2);
    for(int i = customMessageLine2.length(); i < 16; i++) lcd.print(" ");
  } else {
    lcd.setCursor(0, 1);
    lcd.print("                ");
  }
}

// Style 3: Scrolling Left to Right (NO @ support)
void displayStyle3() {
  lcd.setCursor(0, 0);
  String displayMsg = scrollText.substring(animationPosition, animationPosition + 16);
  if(displayMsg.length() < 16) {
    displayMsg += scrollText.substring(0, 16 - displayMsg.length());
  }
  lcd.print(displayMsg);
  lcd.setCursor(0, 1);
  lcd.print("                ");
  
  if(millis() - lastAnimationUpdate > textSpeed) {
    lastAnimationUpdate = millis();
    animationPosition++;
    if(animationPosition >= scrollText.length()) animationPosition = 0;
  }
}

// Style 4: Blinking Text (supports @ - both lines blink)
void displayStyle4() {
  if(millis() - lastBlinkUpdate > textSpeed) {
    lastBlinkUpdate = millis();
    blinkState = !blinkState;
  }
  
  if(blinkState) {
    int spaces1 = (16 - customMessage.length()) / 2;
    if(spaces1 < 0) spaces1 = 0;
    lcd.setCursor(spaces1, 0);
    lcd.print(customMessage);
    for(int i = customMessage.length(); i < 16; i++) lcd.print(" ");
    
    if (customMessageLine2.length() > 0) {
      int spaces2 = (16 - customMessageLine2.length()) / 2;
      if(spaces2 < 0) spaces2 = 0;
      lcd.setCursor(spaces2, 1);
      lcd.print(customMessageLine2);
      for(int i = customMessageLine2.length(); i < 16; i++) lcd.print(" ");
    } else {
      lcd.setCursor(0, 1);
      lcd.print("                ");
    }
  } else {
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
  }
}

// Style 5: Wipe Cursor Effect (sequential: line1 first, then line2, centered)
void displayStyle5() {
  if (!animationCompleted) {
    // Stage 0: Animating line 1
    if (animationStage == 0) {
      if(millis() - lastAnimationUpdate > textSpeed) {
        lastAnimationUpdate = millis();
        if(cursorPos <= customMessage.length()) {
          cursorPos++;
        }
        
        // Check if line 1 is complete
        if(cursorPos > customMessage.length()) {
          line1Completed = true;
          animationStage = 1; // Move to line 2
          cursorPos2 = 0;
          lastAnimationUpdate = millis();
        }
      }
      
      // Display line 1 (centered)
      int spaces1 = (16 - customMessage.length()) / 2;
      if(spaces1 < 0) spaces1 = 0;
      lcd.setCursor(spaces1, 0);
      if(cursorPos <= customMessage.length()) {
        lcd.print(customMessage.substring(0, cursorPos));
        if(cursorPos < customMessage.length()) {
          lcd.setCursor(spaces1 + cursorPos, 0);
          lcd.print("_");
        }
      }
      for(int i = spaces1 + cursorPos + 1; i < 16; i++) lcd.print(" ");
      
      // Clear line 2 while line 1 is animating
      lcd.setCursor(0, 1);
      lcd.print("                ");
    }
    
    // Stage 1: Animating line 2
    else if (animationStage == 1 && customMessageLine2.length() > 0) {
      if(millis() - lastAnimationUpdate > textSpeed) {
        lastAnimationUpdate = millis();
        if(cursorPos2 <= customMessageLine2.length()) {
          cursorPos2++;
        }
        
        // Check if line 2 is complete
        if(cursorPos2 > customMessageLine2.length()) {
          line2Completed = true;
          animationCompleted = true;
        }
      }
      
      // Line 1 is already fully displayed
      int spaces1 = (16 - customMessage.length()) / 2;
      if(spaces1 < 0) spaces1 = 0;
      lcd.setCursor(spaces1, 0);
      lcd.print(customMessage);
      for(int i = customMessage.length(); i < 16; i++) lcd.print(" ");
      
      // Display line 2 (centered)
      int spaces2 = (16 - customMessageLine2.length()) / 2;
      if(spaces2 < 0) spaces2 = 0;
      lcd.setCursor(spaces2, 1);
      if(cursorPos2 <= customMessageLine2.length()) {
        lcd.print(customMessageLine2.substring(0, cursorPos2));
        if(cursorPos2 < customMessageLine2.length()) {
          lcd.setCursor(spaces2 + cursorPos2, 1);
          lcd.print("_");
        }
      }
      for(int i = spaces2 + cursorPos2 + 1; i < 16; i++) lcd.print(" ");
    }
    
    // If no second line, just complete after line 1
    else if (animationStage == 1 && customMessageLine2.length() == 0) {
      animationCompleted = true;
    }
  } else {
    // Keep showing the completed text (centered)
    int spaces1 = (16 - customMessage.length()) / 2;
    if(spaces1 < 0) spaces1 = 0;
    lcd.setCursor(spaces1, 0);
    lcd.print(customMessage);
    for(int i = customMessage.length(); i < 16; i++) lcd.print(" ");
    
    if (customMessageLine2.length() > 0) {
      int spaces2 = (16 - customMessageLine2.length()) / 2;
      if(spaces2 < 0) spaces2 = 0;
      lcd.setCursor(spaces2, 1);
      lcd.print(customMessageLine2);
      for(int i = customMessageLine2.length(); i < 16; i++) lcd.print(" ");
    } else {
      lcd.setCursor(0, 1);
      lcd.print("                ");
    }
  }
}

// Style 6: Typewriter Effect (sequential: line1 first, then line2, centered)
void displayStyle6() {
  if (!animationCompleted) {
    // Stage 0: Animating line 1
    if (animationStage == 0) {
      if(millis() - lastCharTime > textSpeed && charCount < customMessage.length()) {
        lastCharTime = millis();
        charCount++;
      }
      
      // Check if line 1 is complete
      if(charCount >= customMessage.length()) {
        line1Completed = true;
        animationStage = 1; // Move to line 2
        charCount2 = 0;
        lastCharTime = millis();
      }
      
      // Display line 1 (centered)
      int spaces1 = (16 - customMessage.length()) / 2;
      if(spaces1 < 0) spaces1 = 0;
      lcd.setCursor(spaces1, 0);
      String displayMsg = customMessage.substring(0, charCount);
      lcd.print(displayMsg);
      for(int i = displayMsg.length(); i < customMessage.length(); i++) lcd.print("_");
      for(int i = spaces1 + customMessage.length(); i < 16; i++) lcd.print(" ");
      
      // Clear line 2 while line 1 is animating
      lcd.setCursor(0, 1);
      lcd.print("                ");
    }
    
    // Stage 1: Animating line 2
    else if (animationStage == 1 && customMessageLine2.length() > 0) {
      if(millis() - lastCharTime > textSpeed && charCount2 < customMessageLine2.length()) {
        lastCharTime = millis();
        charCount2++;
      }
      
      // Check if line 2 is complete
      if(charCount2 >= customMessageLine2.length()) {
        line2Completed = true;
        animationCompleted = true;
      }
      
      // Line 1 is already fully displayed
      int spaces1 = (16 - customMessage.length()) / 2;
      if(spaces1 < 0) spaces1 = 0;
      lcd.setCursor(spaces1, 0);
      lcd.print(customMessage);
      for(int i = customMessage.length(); i < 16; i++) lcd.print(" ");
      
      // Display line 2 (centered)
      int spaces2 = (16 - customMessageLine2.length()) / 2;
      if(spaces2 < 0) spaces2 = 0;
      lcd.setCursor(spaces2, 1);
      String displayMsg2 = customMessageLine2.substring(0, charCount2);
      lcd.print(displayMsg2);
      for(int i = displayMsg2.length(); i < customMessageLine2.length(); i++) lcd.print("_");
      for(int i = spaces2 + customMessageLine2.length(); i < 16; i++) lcd.print(" ");
    }
    
    // If no second line, just complete after line 1
    else if (animationStage == 1 && customMessageLine2.length() == 0) {
      animationCompleted = true;
    }
  } else {
    // Keep showing the completed text (centered)
    int spaces1 = (16 - customMessage.length()) / 2;
    if(spaces1 < 0) spaces1 = 0;
    lcd.setCursor(spaces1, 0);
    lcd.print(customMessage);
    for(int i = customMessage.length(); i < 16; i++) lcd.print(" ");
    
    if (customMessageLine2.length() > 0) {
      int spaces2 = (16 - customMessageLine2.length()) / 2;
      if(spaces2 < 0) spaces2 = 0;
      lcd.setCursor(spaces2, 1);
      lcd.print(customMessageLine2);
      for(int i = customMessageLine2.length(); i < 16; i++) lcd.print(" ");
    } else {
      lcd.setCursor(0, 1);
      lcd.print("                ");
    }
  }
}

void loop() {
  if (SerialBT.available() > 0) {
    String cmd = SerialBT.readStringUntil('\n');
    processCommand(cmd);
  }
  
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    processCommand(cmd);
  }

  if (!systemOn) {
    delay(100);
    return;
  }

  updateDHT();

  if (messageStyle == 0) {
    // Default clock display
    DateTime rtcNow = rtc.now();
    
    int currentMs = millis() % 1000;
    bool showColon = (currentMs < 500);
    
    int hour24 = rtcNow.hour();
    String ampm = (hour24 < 12) ? "AM" : "PM";
    int hour12 = hour24 % 12;
    if (hour12 == 0) hour12 = 12;

    String sep = showColon ? ":" : " ";
    String hStr = (hour12 < 10) ? "0" + String(hour12) : String(hour12);
    String mStr = (rtcNow.minute() < 10) ? "0" + String(rtcNow.minute()) : String(rtcNow.minute());

    int dispTemp = (int)cachedTemp;
    int dispHumidity = (int)cachedHumidity;

    // Line 0: Time and Temperature
    lcd.setCursor(0, 0);
    lcd.print(hStr + sep + mStr + " " + ampm);
    lcd.setCursor(9, 0);
    lcd.print(" T=");
    if (dispTemp < 10) lcd.print(" ");
    lcd.print(dispTemp);
    lcd.print((char)223);
    lcd.print("C");

    // Line 1: Date and Humidity
    String dowStr = String(days[rtcNow.dayOfTheWeek()]);
    String monStr = String(months[rtcNow.month() - 1]);
    String dayDStr = (rtcNow.day() < 10) ? "0" + String(rtcNow.day()) : String(rtcNow.day());

    lcd.setCursor(0, 1);
    lcd.print(monStr);
    lcd.print(dayDStr);
    lcd.print(" ");
    lcd.print(dowStr);
    lcd.print("  H=");
    if (dispHumidity < 10) lcd.print(" ");
    lcd.print(dispHumidity);
    lcd.print("%");
  } else {
    // Display selected text style
    switch(messageStyle) {
      case 1: displayStyle1(); break;
      case 2: displayStyle2(); break;
      case 3: displayStyle3(); break;
      case 4: displayStyle4(); break;
      case 5: displayStyle5(); break;
      case 6: displayStyle6(); break;
    }
  }

  delay(20);
}
