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
int animationSpeed = 500; // Default speed (1-1000, 500 = medium)
int textSpeed = 150; // Default delay in ms
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

// Motivation mode variables
bool motivationMode = false;
int motivationStyle = 0;
unsigned long lastMotivationChange = 0;
int currentMotivationIndex = 0;

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
  // Map speed 1-1000 to delay from 2000ms (very slow) to 10ms (very fast)
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
  
  // Reset animation states
  animationPosition = 0;
  cursorPos = 0;
  cursorPos2 = 0;
  charCount = 0;
  charCount2 = 0;
  animationCompleted = false;
  line1Completed = false;
  line2Completed = false;
  animationStage = 0;
  
  // Get current motivation message
  String msg = String(motivationMessages[currentMotivationIndex]);
  splitMessage(msg);
  
  // Prepare scrolling text for style 3
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
  // Get new random message
  currentMotivationIndex = random(0, totalMessages);
  String msg = String(motivationMessages[currentMotivationIndex]);
  splitMessage(msg);
  
  // Reset animation states
  animationPosition = 0;
  cursorPos = 0;
  cursorPos2 = 0;
  charCount = 0;
  charCount2 = 0;
  animationCompleted = false;
  line1Completed = false;
  line2Completed = false;
  animationStage = 0;
  
  // Prepare scrolling text for style 3
  if (motivationStyle == 3) {
    customMessage = msg;
    customMessageLine2 = "";
    if (customMessage.indexOf('@') != -1) {
      customMessage.replace("@", "");
    }
    scrollText = customMessage + "    ";
  }
  
  lcd.clear();
  Serial.println("→ New motivation message loaded");
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_Clock");
  delay(500);
  
  randomSeed(analogRead(0));

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
  Serial.println("speed(1-1000)             → Set animation speed (1=very slow, 1000=very fast)");
  Serial.println("offme                      → Turn OFF display & system");
  Serial.println("onme                       → Turn ON display & system");
  Serial.println("\n--- TEXT STYLES ---");
  Serial.println("text(1)Your message       → Static Left Aligned (supports @)");
  Serial.println("text(2)Your message       → Center Aligned (supports @)");
  Serial.println("text(3)Your message       → Scrolling Left to Right (NO @ support)");
  Serial.println("text(4)Your message       → Blinking Text (supports @)");
  Serial.println("text(5)Your message       → Wipe Cursor Effect (sequential, centered)");
  Serial.println("text(6)Your message       → Typewriter Effect (sequential, centered)");
  Serial.println("\n--- MOTIVATION MODE ---");
  Serial.println("mv(2)                     → Motivation mode with Style 2 (changes every 25 sec)");
  Serial.println("mv(4)                     → Motivation mode with Style 4");
  Serial.println("mv(5)                     → Motivation mode with Style 5");
  Serial.println("mv(6)                     → Motivation mode with Style 6");
  Serial.println("\nUse @ for two-line display (e.g., Rahul@Manna)");
  Serial.println("Just type any text (e.g., RAHUL) → Centered static text (Style 2)");
  Serial.println("home                       → Back to clock\n");
  
  Serial.printf("Current speed: %d/1000 (delay: %dms)\n", animationSpeed, textSpeed);
  Serial.printf("Total motivation messages loaded: %d\n", totalMessages);

  lcd.setCursor(0, 0);
  lcd.print("RAHUL'S  CLOCK");
  lcd.setCursor(3, 1);
  lcd.print("ESP32  v14.0");
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
    motivationMode = false;
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
    motivationMode = false;
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
    motivationMode = false;
    Serial.println("→ System ON");
    SerialBT.println("→ System ON");
    return;
  }
  
  // Motivation mode command
  if (cmd.startsWith("mv(")) {
    int closeParen = cmd.indexOf(')');
    if (closeParen != -1) {
      int style = cmd.substring(3, closeParen).toInt();
      if (style == 2 || style == 4 || style == 5 || style == 6) {
        startMotivationMode(style);
      } else {
        Serial.println("→ Invalid style for motivation! Use 2, 4, 5, or 6");
        SerialBT.println("→ Invalid style for motivation! Use 2, 4, 5, or 6");
      }
    }
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
      if (speed >= 1 && speed <= 1000) {
        animationSpeed = speed;
        updateSpeed();
        Serial.printf("→ Speed: %d/1000 (delay: %dms)\n", animationSpeed, textSpeed);
        SerialBT.printf("→ Speed: %d/1000\n", animationSpeed);
      } else {
        Serial.println("→ Speed must be between 1-1000");
        SerialBT.println("→ Speed must be between 1-1000");
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
        motivationMode = false;
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
        Serial.printf(" (speed: %dms delay)\n", textSpeed);
        SerialBT.printf("→ Style %d activated\n", style);
      }
    }
    return;
  }
  
  // Normal text command with @ support
  if (!cmd.startsWith("SET:") && cmd.length() > 0 && cmd != "home" && 
      !cmd.startsWith("text(") && !cmd.startsWith("bright(") && 
      !cmd.startsWith("speed(") && cmd != "offme" && cmd != "onme" &&
      !cmd.startsWith("mv(")) {
    motivationMode = false;
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

  // Handle motivation mode timer
  if (motivationMode && messageStyle != 0) {
    if (millis() - lastMotivationChange > 25000) { // 25 seconds
      lastMotivationChange = millis();
      changeMotivationMessage();
    }
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

  delay(10); // Small delay for stability
}
