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

// Button pins
#define MODE_SELECT_PIN     32
#define SET_CONFIRM_PIN     33
#define INCREASE_PIN        25
#define DECREASE_PIN        26
#define DISCARD_PIN         27
#define BUZZER_FREQ       1800
// Buzzer pin
#define BUZZER_PIN          12

int brightnessLevel = 500;
String customMessage = "";
String customMessageLine2 = "";
int messageStyle = 0;
unsigned long lastAnimationUpdate = 0;
int animationPosition = 0;
String scrollText = "";
bool blinkState = true;
unsigned long lastBlinkUpdate = 0;
int animationSpeed = 500;
int textSpeed = 150;
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
int animationStage = 0;

// Motivation mode variables
bool motivationMode = false;
int motivationStyle = 0;
unsigned long lastMotivationChange = 0;
int currentMotivationIndex = 0;

// Mode variables
enum SystemMode { CLOCK_MODE, ALARM_MODE, TIMER_MODE };
SystemMode currentMode = CLOCK_MODE;
bool showModeSelectMessage = false;
unsigned long modeMessageStartTime = 0;

// Alarm variables
bool alarmActive = false;
int alarmHour = 8;
int alarmMinute = 0;
int alarmSecond = 0;
bool alarmTriggered = false;
unsigned long lastAlarmBeep = 0;
bool alarmBlinkState = false;
unsigned long lastAlarmBlink = 0;
unsigned long lastAlarmSecondBeep = 0;  // For per-second beep in alarm mode
int lastDisplayAlarmSecond = -1;         // Track last second displayed

// Timer variables
bool timerRunning = false;
unsigned long timerStartTime = 0;
unsigned long timerRemaining = 0;
int timerHours = 0;
int timerMinutes = 1;
int timerSeconds = 0;
bool timerExpired = false;
unsigned long lastTimerBeep = 0;
bool timerBlinkState = false;
int lastDisplaySecond = -1;

// Button debouncing
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 200;

LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;
DHT dht(DHT_PIN, DHT_TYPE);
BluetoothSerial SerialBT;

const char* days[]   = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
const char* months[] = {"Jan","Feb","Mar","Apr","May","Jun",
                        "Jul","Aug","Sep","Oct","Nov","Dec"};

// Motivation messages array
const char* motivationMessages[] = {
  "focus on logic@build the future",
  "crack the gate@reach the iit tag",
  "code your way@to the top tier",
  "master discrete@math for the win",
  "dream of iit@work for it now",
  "os concepts are@key to success",
  "solve the pyqs@rank will follow",
  "data structures@fix all tasks",
  "be a topper in@cs engineering",
  "algo design is@your best tool",
  "compilers are@logic engines",
  "dbms mastery is@within reach",
  "keep on coding@keep climbing",
  "gate exam 2027@is your big goal",
  "believe in the@cs power today",
  "revise daily@retain it weekly",
  "toc is hard@but you are too",
  "network layers@lead to glory",
  "stay calm now@solve the gate",
  "iit dreams are@made of grit",
  "digital logic@is very sharp",
  "paging is easy@if you study",
  "graphs lead to@the right path",
  "trees grow with@your efforts",
  "success is an@o1 operation",
  "your potential@has no limits",
  "aptitude is a@scoring zone",
  "logic gates@open iit doors",
  "stay hungry@stay a true guy",
  "rank under 100@is the target",
  "dont stop till@you reach iit",
  "math is the@base of all cs",
  "practice makes@you a topper",
  "study very hard@code smarter",
  "one byte at@time you win it",
  "recursion leads@to the peak",
  "sort your life@sort your rank",
  "binary search@for your goal",
  "hashing out@all the trouble",
  "cache your@knowledge well",
  "pointers point@to the trophy",
  "deadlock free@study mode now",
  "threads of@hard work pay",
  "sql queries@unlock the data",
  "be the master@of the automata",
  "context free@focus is the key",
  "p vs np is@a big mind game",
  "queue up for@the biggest win",
  "stack up marks@for today work",
  "linked lists@lead to success",
  "heaps of joy@await you at iit",
  "min cut for@all obstacles",
  "max flow for@your own energy",
  "be an expert@in the c coding",
  "linux kernels@are a cool art",
  "learn tcp for@the better link",
  "udp is fast@be just like that",
  "ip address of@success is iit",
  "routing your@way to glory",
  "bit by bit@you will win it",
  "boolean algebra@never lies",
  "k maps simplify@your own life",
  "mux your time@for the result",
  "pipeline your@study flow now",
  "no stalls in@your progress",
  "branch out to@the new height",
  "risc it all@for the dream",
  "cisc is big@just stay focused",
  "memory manage@is very vital",
  "virtual ram@for real success",
  "semaphores for@clear paths",
  "mutex for your@focus time now",
  "deadly focus@no deadlocks",
  "b trees help@you to search",
  "b plus trees@for dense ranks",
  "normalise your@study plan now",
  "1nf to bcnf@know it all well",
  "acid traits@in your own work",
  "greedy algos@for the marks",
  "dynamic coding@for the win",
  "divide and@conquer the gate",
  "backtrack to@fix the error",
  "complexity is@just a number",
  "big o of n@is the target",
  "stay linear@stay focused",
  "exponentially@grow every day",
  "log time is@the best time",
  "np hard is@not really hard",
  "sat solvers@fix the gate",
  "grammar leads@to languages",
  "pushdown your@fears today",
  "turing machine@of hard work",
  "decide to be@the best one",
  "halting is@not an option",
  "learn to code@code to learn",
  "cs is the@future of all",
  "gate is just@one step away",
  "iit bombay is@now calling",
  "iit delhi is@your vision",
  "iit madras is@the target",
  "iit kanpur@awaits you now",
  "iit kharagpur@is the goal",
  "iit roorkee@awaits logic",
  "iit guwahati@looks so great",
  "iisc is the@ultimate aim",
  "research is@your passion",
  "coding is your@super power",
  "stay humble@work much harder",
  "silence your@doubts now",
  "errors lead@to perfection",
  "debug your@own life path",
  "compile your@strength now",
  "run the code@of success",
  "execute your@plans well",
  "input effort@output iit seat",
  "no bugs in@your strategy",
  "logic is the@only weapon",
  "maths is the@queen of gate",
  "probability@of winning one",
  "stats prove@you are good",
  "matrices are@easy points",
  "eigen values@of success",
  "calculus for@steady growth",
  "groups and@rings of power",
  "graph theory@is beautiful",
  "master bayes@for the gate",
  "expectation@is to top gate",
  "variance in@study is bad",
  "stay constant@stay very ready",
  "uniformly work@every day",
  "normalise the@pressure now",
  "poisson flow@of the wins",
  "binomial luck@comes to all",
  "set theory@is the start",
  "functions are@your friends",
  "relations last@long in gate",
  "lattices and@posets win it",
  "groups define@your circle",
  "be a real pro@in proposition",
  "predicates@are true for you",
  "quantify your@hard effort",
  "validity is@your strength",
  "satisfy all@the gate rules",
  "complexity@of dreams high",
  "paging the@future today",
  "segmentation@of your goals",
  "dirty bits@in your prep",
  "write back@your success",
  "write through@the struggle",
  "tlb miss is@just a slip",
  "hit ratio of@ninety percent",
  "locality of@focus is key",
  "temporal grit@spacial aim",
  "bus width@of your brain",
  "clock speed@of your logic",
  "cycles of@constant study",
  "fetch the@greatest rank",
  "decode the@gate pattern",
  "alu of your@mind is fast",
  "registers@of memory stay",
  "direct mapped@to the goal",
  "set associative@winning",
  "fully linked@to the dream",
  "page faults@teach lessons",
  "thrashing is@not allowed",
  "working set@of winners",
  "beladys luck@will not stop",
  "optimal prep@leads to iit",
  "lru means@learn recent",
  "fifo for@all your tasks",
  "sjf for your@short goals",
  "round robin@your subjects",
  "priority is@always gate",
  "multi level@success plan",
  "deadlock is@for the weak",
  "bankers logic@keeps safety",
  "safety state@is iit seat",
  "avoid all@the wrong paths",
  "prevent any@lazy habits",
  "detection@of weak areas",
  "recovery@from failures",
  "disk space@for big ideas",
  "raid your@way to glory",
  "striping@the competition",
  "mirrored@in your efforts",
  "checksum for@your growth",
  "parity with@the toppers",
  "coding theory@is so fun",
  "hamming way@to perfection",
  "distance@to iit is short",
  "error free@mindset now",
  "cycles of@constant win",
  "paths lead@to the summit",
  "adjacency@to greatness",
  "degree of@success is one",
  "connected@to your dreams",
  "isomorphic@to a winner",
  "planar graphs@of progress",
  "euler path@to the goal",
  "hamiltonian@strength now",
  "cliques of@top engineers",
  "cover all@the syllabus",
  "matching@your ambition",
  "flow through@the hurdles",
  "source is@your hard work",
  "sink is the@final iit",
  "reach for@the top rank",
  "failures are@the beta test",
  "sleep is@for non gate guy",
  "coffee runs@in my veins",
  "social life@segment fault",
  "iit tag or@just a rag",
  "rank decides@your worth",
  "dont cry@just dry run",
  "logic high@feelings low",
  "love is a@recursive trap",
  "no gf only@bf best friend",
  "study now@flex later on",
  "job is safe@but iit is fire",
  "be the zero@one percent guy",
  "pain is just@a data input",
  "cry in jaguar@not in bus",
  "iit air@feels different",
  "society@wants your rank",
  "parents want@that iit tag",
  "neighbors envy@is my goal",
  "ex gf will@regret soon",
  "burn the@midnight oil",
  "grind now@shine forever",
  "gate is a@battlefield",
  "be the king@of core stuff",
  "tech is@the new gold",
  "silicon valley@calls you",
  "start up is@a post iit",
  "think like@a machine",
  "be cold as@a processor",
  "heart is just@a pump chill",
  "emotions@are run errors",
  "hard work@beats genius",
  "no shortcut@to the top",
  "stairs to@iit are steep",
  "sweat in peace@win the war",
  "focus is@my superpower",
  "discipline@is the key",
  "consistent@is the word",
  "beat all@the odds today",
  "impossible@is logic error",
  "win the race@be the ace",
  "top rank@or nothing now",
  "eat sleep@gate repeat",
  "library is@my second home",
  "books are@my only bae",
  "pen is my@only weapon",
  "notes are@my treasure",
  "summary@is not enough",
  "deep dive@into syllabus",
  "zero days@off allowed",
  "one goal@and one vision",
  "eyes on the@the iit gate",
  "push limits@every day",
  "break barriers@not hearts",
  "silence is@the best noise",
  "success@will make a roar",
  "prove them@all wrong now",
  "watch me@reach the top",
  "history@is made by us",
  "be a legend@in the cs field",
  "future ceo@in the making",
  "code the@world better",
  "innovate or@get deleted",
  "stay hungry@stay foolish",
  "vision of@a top topper",
  "mindset of@a conqueror",
  "born to@crack the gate",
  "destined@for iit glory",
  "master of@all subjects",
  "king of the@the cs kingdom",
  "rule the@tech world",
  "binary@is my language",
  "hex life@is a rich life",
  "logic over@everything",
  "reasoning@is my strength",
  "puzzle solver@by birth",
  "born for@the engineer",
  "building@the next thing",
  "dream big@act even bigger",
  "small steps@for big impact",
  "giant leap@for my career",
  "path to@the iit is clear",
  "walk the@talk every day",
  "no excuses@only results",
  "kill the@laziness now",
  "destroy@distractions",
  "focus like@a laser beam",
  "sharp mind@and sharp rank",
  "bright future@awaits you",
  "be the light@of your home",
  "pride of@the college",
  "star of@the family tree",
  "legacy starts@with the gate",
  "iit is@just beginning",
  "life begins@at iit gates",
  "magic happens@in the lab",
  "creation@is my passion",
  "art of@problem solving",
  "science@of winning life",
  "philosophy@of a topper",
  "way of the@brave warrior",
  "code of@conduct to win",
  "protocol@for success",
  "standards@are set high",
  "quality@over quantity",
  "precision@is everything",
  "accuracy@is the aim",
  "speed is@the only edge",
  "efficiency@is the goal",
  "optimize@your life now",
  "refactor@your habits",
  "upgrade@your circle",
  "system update@is required",
  "reboot@your spirit now",
  "full charge@mode is on",
  "unlimited@true potential",
  "infinite@possibilities",
  "end game@is iit madras",
  "final boss@is gate exam",
  "level up@every week",
  "xp gain@from every mock",
  "skill points@on the maths",
  "inventory@of formulae",
  "quest for@the iit tag",
  "mission@is accomplished",
  "victory@is my destiny",
  "crown of@a gate topper",
  "glory is@truly eternal",
  "hard work@will never fade",
  "legendary@status is aim",
  "epic win@in the feb exam",
  "coolest geek@in the town",
  "sartorial@the iit hoodie",
  "brand of@an iit student",
  "worth of@a top scholar",
  "value of@the discipline",
  "price of@the greatness",
  "cost of@the mediocrity",
  "avoid the@average life",
  "be the@extraordinary",
  "phenomenal@rank is coming",
  "stellar@performance",
  "galactic@huge ambition",
  "universal@recognition",
  "beyond@the far horizon",
  "sky is@not the limit",
  "iit and@far beyond gate",
  "future@is in your hands",
  "write your@own story now",
  "be the@true protagonist",
  "hero of@your journey",
  "success@is coming home",
  "iit is@calling you now",
  "wait for@the result day",
  "tears of@joy very soon",
  "smile of@a winner now",
  "peace at@the highest peak",
  "the end@of the grind",
  "start of@a new life",
  "welcome@to the iit life"
};

const int totalMessages = sizeof(motivationMessages) / sizeof(motivationMessages[0]);

int scaleToPWM(int level) {
  if (level <= 1)    return 0;
  if (level >= 1000) return 255;
  return map(level, 1, 1000, 1, 255);
}

void setBacklight(int level) {
  if (!systemOn) level = 0;
  ledcWrite(BACKLIGHT_PIN, scaleToPWM(level));
}

void updateSpeed() {
  textSpeed = map(animationSpeed, 1, 1000, 2000, 10);
  if (textSpeed < 5) textSpeed = 5;
  if (textSpeed > 2000) textSpeed = 2000;
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

void startMotivationMode(int style) {
  motivationMode = true;
  motivationStyle = style;
  messageStyle = style;
  currentMotivationIndex = random(0, totalMessages);
  lastMotivationChange = millis();
  
  animationPosition = 0;
  cursorPos = 0;
  cursorPos2 = 0;
  charCount = 0;
  charCount2 = 0;
  animationCompleted = false;
  line1Completed = false;
  line2Completed = false;
  animationStage = 0;
  
  String msg = String(motivationMessages[currentMotivationIndex]);
  splitMessage(msg);
  
  if (style == 3) {
    customMessage = msg;
    customMessageLine2 = "";
    if (customMessage.indexOf('@') != -1) {
      customMessage.replace("@", "");
    }
    scrollText = customMessage + "    ";
  }
  
  lcd.clear();
  Serial.printf("→ Motivation Mode Started (Style %d)\n", style);
  SerialBT.printf("→ Motivation Mode Started (Style %d)\n", style);
}

void changeMotivationMessage() {
  currentMotivationIndex = random(0, totalMessages);
  String msg = String(motivationMessages[currentMotivationIndex]);
  splitMessage(msg);
  
  animationPosition = 0;
  cursorPos = 0;
  cursorPos2 = 0;
  charCount = 0;
  charCount2 = 0;
  animationCompleted = false;
  line1Completed = false;
  line2Completed = false;
  animationStage = 0;
  
  if (motivationStyle == 3) {
    customMessage = msg;
    customMessageLine2 = "";
    if (customMessage.indexOf('@') != -1) {
      customMessage.replace("@", "");
    }
    scrollText = customMessage + "    ";
  }
  
  lcd.clear();
}

void buzzerBeep() {
  tone(BUZZER_PIN, BUZZER_FREQ);
  delay(100);
  noTone(BUZZER_PIN);
}

void buzzerShortBeep() {
  tone(BUZZER_PIN, BUZZER_FREQ);
  delay(50);
  noTone(BUZZER_PIN);
}

void buzzerContinuousStart() {
  tone(BUZZER_PIN, BUZZER_FREQ);
}

void buzzerContinuousStop() {
  noTone(BUZZER_PIN);
}

void setupAlarm() {
  int settingStep = 0;
  int tempHour = alarmHour;
  int tempMinute = alarmMinute;
  int tempSecond = alarmSecond;
  bool settingComplete = false;
  unsigned long lastBlink = 0;
  bool showCursor = true;
  
  lcd.clear();
  
  while (!settingComplete && currentMode == ALARM_MODE) {
    if (digitalRead(DISCARD_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis();
      currentMode = CLOCK_MODE;
      lcd.clear();
      return;
    }
    
    if (digitalRead(MODE_SELECT_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis();
      currentMode = CLOCK_MODE;
      lcd.clear();
      return;
    }
    
    if (millis() - lastButtonPress > debounceDelay) {
      if (digitalRead(INCREASE_PIN) == LOW) {
        lastButtonPress = millis();
        switch(settingStep) {
          case 0: tempHour = (tempHour + 1) % 24; break;
          case 1: tempMinute = (tempMinute + 1) % 60; break;
          case 2: tempSecond = (tempSecond + 1) % 60; break;
        }
      }
      
      if (digitalRead(DECREASE_PIN) == LOW) {
        lastButtonPress = millis();
        switch(settingStep) {
          case 0: tempHour = (tempHour - 1 + 24) % 24; break;
          case 1: tempMinute = (tempMinute - 1 + 60) % 60; break;
          case 2: tempSecond = (tempSecond - 1 + 60) % 60; break;
        }
      }
      
      if (digitalRead(SET_CONFIRM_PIN) == LOW) {
        lastButtonPress = millis();
        if (settingStep < 2) {
          settingStep++;
        } else {
          alarmHour = tempHour;
          alarmMinute = tempMinute;
          alarmSecond = tempSecond;
          alarmActive = true;
          alarmTriggered = false;
          settingComplete = true;
          lcd.clear();
          return;
        }
      }
    }
    
    lcd.setCursor(0, 0);
    lcd.print("SET ALARM TIME");
    
    lcd.setCursor(0, 1);
    
    if (millis() - lastBlink > 500) {
      lastBlink = millis();
      showCursor = !showCursor;
    }
    
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", tempHour, tempMinute, tempSecond);
    lcd.print(timeStr);
    
    if (showCursor) {
      lcd.setCursor(settingStep * 3, 1);
      lcd.print("  ");
      lcd.setCursor(settingStep * 3, 1);
    }
    
    delay(50);
  }
}

void setupTimer() {
  int settingStep = 0;
  int tempHour = timerHours;
  int tempMinute = timerMinutes;
  int tempSecond = timerSeconds;
  bool settingComplete = false;
  unsigned long lastBlink = 0;
  bool showCursor = true;
  
  lcd.clear();
  
  while (!settingComplete && currentMode == TIMER_MODE) {
    if (digitalRead(DISCARD_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis();
      currentMode = CLOCK_MODE;
      lcd.clear();
      return;
    }
    
    if (digitalRead(MODE_SELECT_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis();
      currentMode = CLOCK_MODE;
      lcd.clear();
      return;
    }
    
    if (millis() - lastButtonPress > debounceDelay) {
      if (digitalRead(INCREASE_PIN) == LOW) {
        lastButtonPress = millis();
        switch(settingStep) {
          case 0: tempHour = (tempHour + 1) % 24; break;
          case 1: tempMinute = (tempMinute + 1) % 60; break;
          case 2: tempSecond = (tempSecond + 1) % 60; break;
        }
      }
      
      if (digitalRead(DECREASE_PIN) == LOW) {
        lastButtonPress = millis();
        switch(settingStep) {
          case 0: tempHour = (tempHour - 1 + 24) % 24; break;
          case 1: tempMinute = (tempMinute - 1 + 60) % 60; break;
          case 2: tempSecond = (tempSecond - 1 + 60) % 60; break;
        }
      }
      
      if (digitalRead(SET_CONFIRM_PIN) == LOW) {
        lastButtonPress = millis();
        if (settingStep < 2) {
          settingStep++;
        } else {
          timerHours = tempHour;
          timerMinutes = tempMinute;
          timerSeconds = tempSecond;
          timerRemaining = (timerHours * 3600L + timerMinutes * 60 + timerSeconds) * 1000L;
          timerRunning = false;
          timerExpired = false;
          settingComplete = true;
          lcd.clear();
          runTimer();
          return;
        }
      }
    }
    
    lcd.setCursor(0, 0);
    lcd.print("SET TIMER");
    
    lcd.setCursor(0, 1);
    
    if (millis() - lastBlink > 500) {
      lastBlink = millis();
      showCursor = !showCursor;
    }
    
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", tempHour, tempMinute, tempSecond);
    lcd.print(timeStr);
    
    if (showCursor) {
      lcd.setCursor(settingStep * 3, 1);
      lcd.print("  ");
      lcd.setCursor(settingStep * 3, 1);
    }
    
    delay(50);
  }
}

void runTimer() {
  lcd.clear();
  timerRunning = true;
  timerExpired = false;
  timerStartTime = millis();
  lastDisplaySecond = -1;
  
  while (currentMode == TIMER_MODE && !timerExpired) {
    if (digitalRead(DISCARD_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis();
      timerRunning = false;
      timerExpired = false;
      buzzerContinuousStop();
      currentMode = CLOCK_MODE;
      lcd.clear();
      return;
    }
    
    if (digitalRead(MODE_SELECT_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis();
      timerRunning = false;
      timerExpired = false;
      buzzerContinuousStop();
      currentMode = CLOCK_MODE;
      lcd.clear();
      return;
    }
    
    if (millis() - lastButtonPress > debounceDelay) {
      if (digitalRead(SET_CONFIRM_PIN) == LOW) {
        lastButtonPress = millis();
        if (timerRunning) {
          timerRunning = false;
          timerRemaining = timerRemaining - (millis() - timerStartTime);
        } else {
          timerStartTime = millis();
          timerRunning = true;
        }
      }
    }
    
    if (timerRunning && !timerExpired) {
      long elapsed = millis() - timerStartTime;
      if (elapsed >= timerRemaining) {
        timerExpired = true;
        timerRunning = false;
        timerRemaining = 0;
        lastTimerBeep = millis();
        buzzerContinuousStart();
      } else {
        timerRemaining = timerRemaining - elapsed;
        timerStartTime = millis();
      }
    }
    
    DateTime now = rtc.now();
    int currentSecond = now.second();
    
    if (timerRunning && !timerExpired) {
      if (currentSecond != lastDisplaySecond) {
        lastDisplaySecond = currentSecond;
        buzzerBeep();
      }
    }
    
    unsigned long displayRemaining = timerRemaining;
    
    int hours = displayRemaining / 3600000;
    int minutes = (displayRemaining % 3600000) / 60000;
    int seconds = (displayRemaining % 60000) / 1000;
    
    lcd.setCursor(0, 0);
    lcd.print("TIMER: ");
    if (hours > 0) {
      char timeStr[9];
      sprintf(timeStr, "%02d:%02d:%02d", hours, minutes, seconds);
      lcd.print(timeStr);
    } else {
      char timeStr[6];
      sprintf(timeStr, "%02d:%02d", minutes, seconds);
      lcd.print(timeStr);
    }
    lcd.print("        ");
    
    lcd.setCursor(0, 1);
    lcd.print("NOW: ");
    char nowStr[9];
    sprintf(nowStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    lcd.print(nowStr);
    
    delay(10);
  }
  
  // ── TIMER "TIME UP" LOOP ──────────────────────────────────────────────────
  while (timerExpired && currentMode == TIMER_MODE) {
    if (digitalRead(DISCARD_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis();
      buzzerContinuousStop();
      timerExpired = false;
      currentMode = CLOCK_MODE;
      lcd.clear();
      return;
    }
    
    if (digitalRead(MODE_SELECT_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis();
      buzzerContinuousStop();
      timerExpired = false;
      currentMode = CLOCK_MODE;
      lcd.clear();
      return;
    }
    
    if (millis() - lastTimerBeep > 500) {
      lastTimerBeep = millis();
      timerBlinkState = !timerBlinkState;
    }
    
    lcd.setCursor(0, 0);
    if (timerBlinkState) {
      lcd.print("    TIME UP!    ");
    } else {
      lcd.print("                ");
    }
    
    lcd.setCursor(0, 1);
    DateTime now = rtc.now();
    char nowStr[9];
    sprintf(nowStr, "NOW: %02d:%02d:%02d", now.hour(), now.minute(), now.second());
    lcd.print(nowStr);
    
    delay(10);
  }
}

// ── ALARM CHECK (exact-second miss fix) ──────────────────────────────────────
void checkAlarm() {
    if (!alarmActive || alarmTriggered) return;
    
    DateTime now = rtc.now();
    
    // অ্যালার্ম ট্রিগার করার সময় সঠিক করুন
    if (now.hour() == alarmHour &&
        now.minute() == alarmMinute &&
        now.second() >= alarmSecond) {
        alarmTriggered = true;
        alarmActive = false;
        lastAlarmBeep = millis();
        lastAlarmBlink = millis();
        buzzerContinuousStart();
        lcd.clear();  // LCD ক্লিয়ার করুন নতুন মেসেজ দেখানোর জন্য
    }
}

// ── ALARM MODE DISPLAY WITH PER-SECOND BEEP ────────────────────────────────
void displayAlarmMode() {
    DateTime now = rtc.now();
    int currentSecond = now.second();
    
    // Beep every second when alarm is active (not triggered yet)
    if (alarmActive && !alarmTriggered) {
        if (currentSecond != lastDisplayAlarmSecond) {
            lastDisplayAlarmSecond = currentSecond;
            buzzerShortBeep();
        }
    }
    
    lcd.setCursor(0, 0);
    lcd.print("ALARM: ");
    char alarmStr[9];
    sprintf(alarmStr, "%02d:%02d:%02d", alarmHour, alarmMinute, alarmSecond);
    lcd.print(alarmStr);
    lcd.print("  ");
    
    lcd.setCursor(0, 1);
    lcd.print("NOW: ");
    char nowStr[9];
    sprintf(nowStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    lcd.print(nowStr);
    lcd.print("  ");
}

// ── ALARM "TIME UP" LOOP ────────────────────────────────────────────────
void displayAlarmScreen() {
    unsigned long lastBlinkTime = 0;
    bool blinkState = false;
    
    while (alarmTriggered && currentMode == ALARM_MODE) {
        unsigned long currentMillis = millis();
        
        // DISCARD বাটন চেক করা
        if (digitalRead(DISCARD_PIN) == LOW && currentMillis - lastButtonPress > debounceDelay) {
            lastButtonPress = currentMillis;
            buzzerContinuousStop();
            alarmTriggered = false;
            currentMode = CLOCK_MODE;
            lcd.clear();
            return;
        }
        
        // MODE_SELECT বাটন চেক করা
        if (digitalRead(MODE_SELECT_PIN) == LOW && currentMillis - lastButtonPress > debounceDelay) {
            lastButtonPress = currentMillis;
            buzzerContinuousStop();
            alarmTriggered = false;
            currentMode = CLOCK_MODE;
            lcd.clear();
            return;
        }
        
        // TIME UP! টেক্সট ব্লিং করার জন্য (প্রতি 500ms)
        if (currentMillis - lastBlinkTime >= 500) {
            lastBlinkTime = currentMillis;
            blinkState = !blinkState;
        }
        
        // LCD তে TIME UP! দেখানো
        lcd.setCursor(0, 0);
        if (blinkState) {
            lcd.print("    TIME UP!    ");
        } else {
            lcd.print("                ");
        }
        
        // বর্তমান সময় দেখানো (২য় লাইনে)
        lcd.setCursor(0, 1);
        DateTime now = rtc.now();
        char nowStr[17];
        sprintf(nowStr, "NOW: %02d:%02d:%02d", now.hour(), now.minute(), now.second());
        lcd.print(nowStr);
        
        delay(10);
    }
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_Clock");
  delay(500);
  
  randomSeed(analogRead(0));
  
  pinMode(MODE_SELECT_PIN, INPUT_PULLUP);
  pinMode(SET_CONFIRM_PIN, INPUT_PULLUP);
  pinMode(INCREASE_PIN, INPUT_PULLUP);
  pinMode(DECREASE_PIN, INPUT_PULLUP);
  pinMode(DISCARD_PIN, INPUT_PULLUP);
  
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

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

  lcd.setCursor(0, 0);
  lcd.print("RAHUL'S  CLOCK");
  lcd.setCursor(3, 1);
  lcd.print("ESP32  v16.0");
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

void loop() {
  if (SerialBT.available() > 0) {
    String cmd = SerialBT.readStringUntil('\n');
    cmd.trim();
    if (cmd == "home") {
      currentMode = CLOCK_MODE;
      alarmTriggered = false;
      timerExpired = false;
      buzzerContinuousStop();
      lcd.clear();
    }
  }

  if (millis() - lastButtonPress > debounceDelay) {
    if (digitalRead(MODE_SELECT_PIN) == LOW) {
      lastButtonPress = millis();
      
      if (currentMode == CLOCK_MODE) {
        currentMode = ALARM_MODE;
        showModeSelectMessage = true;
        modeMessageStartTime = millis();
        lastDisplayAlarmSecond = -1;  // Reset second tracker
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" -ALARM MODE-  ");
      } 
      else if (currentMode == ALARM_MODE) {
        currentMode = TIMER_MODE;
        showModeSelectMessage = true;
        modeMessageStartTime = millis();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" -TIMER MODE-  ");
      } 
      else if (currentMode == TIMER_MODE) {
        currentMode = CLOCK_MODE;
        showModeSelectMessage = false;
        timerRunning = false;
        timerExpired = false;
        buzzerContinuousStop();
        lcd.clear();
      }
    }
    
    if (digitalRead(DISCARD_PIN) == LOW && currentMode != CLOCK_MODE) {
      lastButtonPress = millis();
      currentMode = CLOCK_MODE;
      alarmTriggered = false;
      timerExpired = false;
      timerRunning = false;
      buzzerContinuousStop();
      lcd.clear();
    }
  }

  if (showModeSelectMessage && (millis() - modeMessageStartTime > 1500)) {
    showModeSelectMessage = false;
    lcd.clear();
  }

  if (!systemOn) {
    delay(100);
    return;
  }

  // Alarm triggered হলে সেই loop-এই থাকবে (timer TIME UP-এর মতো)
  if (alarmTriggered) {
    displayAlarmScreen();
    delay(10);
    return;
}

  if (currentMode == ALARM_MODE) {
    if (!showModeSelectMessage) {
      if (digitalRead(SET_CONFIRM_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
        lastButtonPress = millis();
        setupAlarm();
        lcd.clear();
      }
      
      if (alarmActive && !alarmTriggered) {
        checkAlarm();
        displayAlarmMode();  // This will also handle the per-second beep
      } else if (!alarmActive) {
        lcd.setCursor(0, 0);
        lcd.print(" -ALARM MODE-  ");
        lcd.setCursor(0, 1);
        lcd.print(" ENTER TO SET  ");
      }
    }
    delay(10);
    return;
  }
  
  if (currentMode == TIMER_MODE) {
    if (!showModeSelectMessage) {
      if (digitalRead(SET_CONFIRM_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
        lastButtonPress = millis();
        setupTimer();
        lcd.clear();
      } else {
        lcd.setCursor(0, 0);
        lcd.print(" -TIMER MODE-  ");
        lcd.setCursor(0, 1);
        if (timerHours == 0 && timerMinutes == 1 && timerSeconds == 0 && !timerRunning && !timerExpired) {
          lcd.print(" ENTER TO SET  ");
        } else if (!timerRunning && !timerExpired && timerRemaining > 0) {
          char timeStr[9];
          int hours = timerRemaining / 3600000;
          int minutes = (timerRemaining % 3600000) / 60000;
          int seconds = (timerRemaining % 60000) / 1000;
          sprintf(timeStr, "%02d:%02d:%02d", hours, minutes, seconds);
          lcd.print(" READY: ");
          lcd.print(timeStr);
        } else {
          lcd.print(" ENTER TO SET  ");
        }
      }
    }
    delay(10);
    return;
  }

  if (currentMode == CLOCK_MODE) {
    updateDHT();

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

    lcd.setCursor(0, 0);
    lcd.print(hStr + sep + mStr + " " + ampm);
    lcd.setCursor(9, 0);
    lcd.print(" T=");
    if (dispTemp < 10) lcd.print(" ");
    lcd.print(dispTemp);
    lcd.print((char)223);
    lcd.print("C");

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
  }

  delay(10);
}
