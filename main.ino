#include <LiquidCrystal.h>
#include <LedControl.h>
#include <EEPROM.h>

const int clk = 13;
const int ld = 12;
const int din = 10;
const int an = 9;
const int rs = 8;
const int en = 2;
const int d4 = 4;
const int d5 = 5;
const int d6 = 6;
const int d7 = 7;
const int vrx = A0;
const int vry = A1;
const int sw = A2;

const int matrixSize = 8;
const int levelSize = 16;

const char* github = "github.com/0xfabian   ";
int aboutIndex = 0;
unsigned long lastAboutDraw;

int joyX;
int joyY;
bool leftPress;
bool rightPress;
bool upPress;
bool downPress;

const int mid = 512;
const int safeZone = 200;

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
LedControl lc(din, clk, ld, 1);

enum GameState
{
  MAIN, SETTINGS, TOP, ABOUT, PLAYING, MAT_LEVEL_EDIT, LCD_LEVEL_EDIT, SOUND_EDIT, GAME_OVER
};

GameState state;

const char* mainStrings[] =
{
  "Play",
  "Settings",
  "Top",
  "About"
};

const char* settingsStrings[] = 
{
  "Mat Level",
  "LCD Level",
  "Sound",
  "Reset Top"
};

int menuIndex = 0;
int viewIndex = 0;

struct SavedData
{
  int matLevel;
  int lcdLevel;
  bool sound;
  int topEntries;
};

SavedData data;

void getInput()
{
  int readX = analogRead(vrx);
  int readY = analogRead(vry);

  int newJoyX;
  int newJoyY;

  if (readX > mid + safeZone)
    newJoyX = 1;
  else if (readX < mid - safeZone)
    newJoyX = -1;
  else
    newJoyX = 0;

  if (readY > mid + safeZone)
    newJoyY = 1;
  else if (readY < mid - safeZone)
    newJoyY = -1;
  else
    newJoyY = 0;

  rightPress = false;
  leftPress = false;
  upPress = false;
  downPress = false;

  if (joyX == 0 && newJoyX == -1)
    leftPress = true;

  if (joyX == 0 && newJoyX == 1)
    rightPress = true;

  if (joyY == 0 && newJoyY == -1)
    upPress = true;

  if (joyY == 0 && newJoyY == 1)
    downPress = true;

  joyX = newJoyX;
  joyY = newJoyY;
}

void setup() 
{
  Serial.begin(9600);

  randomSeed(analogRead(A4));

  EEPROM.get(0, data);

  pinMode(sw, INPUT_PULLUP);
  pinMode(an, OUTPUT);

  analogWrite(an, map(data.lcdLevel, 0, 15, 0, 255));

  lc.shutdown(0, false);
  lc.setIntensity(0, data.matLevel);
  lc.clearDisplay(0);

  lcd.begin(16, 2);
  lcd.print("    Welcome!");

  delay(1000);

  state = MAIN;
  drawMain();
}

void loop() 
{
  getInput();

  switch(state)
  {
    case MAIN:            mainMenu();           break;
    case SETTINGS:        settingsMenu();       break;
    case MAT_LEVEL_EDIT:  matLevelEdit();       break;
    case LCD_LEVEL_EDIT:  lcdLevelEdit();       break;
    case SOUND_EDIT:      soundEdit();          break;
    case PLAYING:         gameLoop();           break;
    case GAME_OVER:       gameOverMenu();       break;
    // case TOP:           topMenu();            break;
    case ABOUT:           aboutMenu();          break;
  }
}

void drawMain()
{
  lcd.clear();

  if(menuIndex == viewIndex)
    lcd.print("> ");

  lcd.print(mainStrings[viewIndex]);

  lcd.setCursor(0, 1);

  if(menuIndex == viewIndex + 1)
    lcd.print("> ");

  lcd.print(mainStrings[viewIndex + 1]);
}

void printData(int index)
{
  switch (index)
  {
    case 0: lcd.print(data.matLevel);   break;
    case 1: lcd.print(data.lcdLevel);   break;
    case 2: 
    {
      if (data.sound)
        lcd.print("On");
      else
        lcd.print("Off");

      break;
    }
    case 3: lcd.print(data.topEntries); break;
  }
}

void drawSettings()
{
  lcd.clear();

  if(menuIndex == viewIndex)
    lcd.print("> ");

  lcd.print(settingsStrings[viewIndex]);

  lcd.setCursor(0, 1);

  if(menuIndex == viewIndex + 1)
    lcd.print("> ");

  lcd.print(settingsStrings[viewIndex + 1]);

  lcd.setCursor(12, 0);
  printData(viewIndex);

  lcd.setCursor(12, 1);
  printData(viewIndex + 1);
}

void mainMenu()
{
  if (upPress)
  {
    if (menuIndex > 0)
    {
      menuIndex--;

      if (menuIndex < viewIndex)
        viewIndex--;

      drawMain();
    }
  }
  else if (downPress)
  {
    if (menuIndex < sizeof(mainStrings) / sizeof(char*) - 1)
    {
      menuIndex++;

      if (menuIndex > viewIndex + 1)
      viewIndex++;

      drawMain();
    }
  }

  if (rightPress)
  {
    switch (menuIndex)
    {
      case 0:   state = PLAYING;  reset();  break;
      case 1:   menuIndex = 0; viewIndex = 0; state = SETTINGS;   drawSettings();   break;
      case 2: break;
      case 3:   state = ABOUT;  aboutIndex = 0; drawAbout(); break;
    }
  }
}

void settingsMenu()
{
  if (upPress)
  {
    if (menuIndex > 0)
    {
      menuIndex--;

      if (menuIndex < viewIndex)
        viewIndex--;

      drawSettings();
    }
  }
  else if (downPress)
  {
    if (menuIndex < sizeof(settingsStrings) / sizeof(char*) - 1)
    {
      menuIndex++;

      if (menuIndex > viewIndex + 1)
      viewIndex++;

      drawSettings();
    }
  }

  if (leftPress)
  {
    menuIndex = 0;
    viewIndex = 0;
    state = MAIN;
    drawMain();
  }

  if (rightPress)
  {
    switch (menuIndex)
    {
      case 0:
      {
        for (int i = 0; i < matrixSize; i++)
          for (int j = 0; j < matrixSize; j++)
            lc.setLed(0, i, j, true);

        state = MAT_LEVEL_EDIT;   
        drawEdit();
    
        break;
      }
      case 1:   state = LCD_LEVEL_EDIT;   drawEdit();       break;
      case 2:   state = SOUND_EDIT;       drawEdit();       break;
      case 3:
      { 
        data.topEntries = 0;      
        EEPROM.put(0, data);  
        drawSettings();
        
        break;
      }
    }
  }
}

void drawAbout()
{
  lastAboutDraw = millis();

  lcd.setCursor(0, 0);
  lcd.print("Bomberman clone ");

  lcd.setCursor(0, 1);

  int next = 22 - aboutIndex;

  if (next >= 16)
    lcd.write(github + aboutIndex, 16);
  else
  {
    lcd.write(github + aboutIndex, next);
    lcd.write(github, 16 - next);
  }
}

void aboutMenu()
{
  if (millis() - lastAboutDraw > 500)
  {
    drawAbout();

    aboutIndex++;

    if(aboutIndex == 22)
      aboutIndex = 0;
  }

  if (leftPress)
  {
    menuIndex = 0;
    viewIndex = 0;
    state = MAIN;
    drawMain();
  }
}

void drawEdit()
{
  int i = viewIndex;
  if (menuIndex != i)
    i++;

  lcd.setCursor(0, i - viewIndex);
  lcd.print(settingsStrings[i]);
  lcd.print("  ");
  lcd.setCursor(11, i - viewIndex);
  lcd.print("[");
  printData(i);
  lcd.print("] ");
}

void matLevelEdit()
{
  if (upPress)
  {
    if (data.matLevel < 15)
    {
      data.matLevel++;
      EEPROM.put(0, data);
      lc.setIntensity(0, data.matLevel);
      drawEdit();
    }
  }
  else if (downPress)
  {
    if (data.matLevel > 0)
    {
      data.matLevel--;
      EEPROM.put(0, data);
      lc.setIntensity(0, data.matLevel);
      drawEdit();
    }
  }

  if (leftPress)
  {
    lc.clearDisplay(0);

    state = SETTINGS;
    drawSettings();
  }
}

void lcdLevelEdit()
{
  if (upPress)
  {
    if (data.lcdLevel < 15)
    {
      data.lcdLevel++;
      EEPROM.put(0, data);
      analogWrite(an, map(data.lcdLevel, 0, 15, 0, 255));
      drawEdit();
    }
  }
  else if (downPress)
  {
    if (data.lcdLevel > 0)
    {
      data.lcdLevel--;
      EEPROM.put(0, data);
      analogWrite(an, map(data.lcdLevel, 0, 15, 0, 255));
      drawEdit();
    }
  }

  if (leftPress)
  {
    state = SETTINGS;
    drawSettings();
  }
}

void soundEdit()
{
  if (upPress || downPress)
  {
    data.sound = !data.sound;
    EEPROM.put(0, data);
    drawEdit();
  }

  if (leftPress)
  {
    state = SETTINGS;
    drawSettings();
  }
}

void drawGameOver()
{
  lcd.clear();

  if(menuIndex == 0)
    lcd.print("> ");

  lcd.print("Play Again");
  lcd.setCursor(0, 1);

  if(menuIndex == 1)
    lcd.print("> ");

  lcd.print("Main Menu");
}

void gameOverMenu()
{
  if (upPress && menuIndex == 1)
  {
      menuIndex--;
      drawGameOver();
  }
  else if(downPress && menuIndex == 0)
  {
    menuIndex++;
    drawGameOver();
  }

  if (rightPress)
  {
    if(menuIndex == 0)
    {
      state = PLAYING;  
      reset();
    }
    else
    {
      menuIndex = 0;
      viewIndex = 0;
      state = MAIN;
      drawMain();
    }
  }
}

enum Tile
{
  NONE,
  HARD_WALL,
  SOFT_WALL,
  FIRE
};

Tile level[levelSize][levelSize];

bool gameOver;

uint8_t playerX;
uint8_t playerY;
uint8_t bombX;
uint8_t bombY;
uint8_t wallsGenerated;
uint8_t wallsDestroyed;

bool playerBlink;
bool bombBlink;
bool fireBlink;
bool bombDown;

unsigned long lastPlayerUpdate;
unsigned long lastPlayerBlink;
unsigned long lastBombBlink;
unsigned long lastFireBlink;
unsigned long lastPlaceTime;
unsigned long lastReset;
unsigned long lastDrawUpdate;

const unsigned long playerUpdateTime = 100;
const unsigned long playerBlinkTime = 50;
const unsigned long bombBlinkTime = 200;
const unsigned long fireBlinkTime = 100;
const unsigned long fireTime = 500;
const unsigned long fuseTime = 3000;

bool triggerDrawUpdate;

const uint8_t gameOverBitmap[] = 
{
  0, 0, 0, 0, 0, 0, 0, 0, 0,
  0x3c, 0x42, 0x42, 0x52, 0x52, 0x34, 0,
  0x20, 0x54, 0x54, 0x54, 0x78, 0,
  0x7c, 0x04, 0x7c, 0x04, 0x78, 0,
  0x38, 0x54, 0x54, 0x54, 0x58, 0, 0, 0,
  0x3c, 0x42, 0x42, 0x42, 0x42, 0x3c, 0,
  0x1c, 0x20, 0x40, 0x20, 0x1c, 0,
  0x38, 0x54, 0x54, 0x54, 0x58, 0,
  0x7c, 0x08, 0x04, 0x04, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};

void generateLevel()
{
  for(int y = 0; y < levelSize; y++)
    for(int x = 0; x < levelSize; x++)
    {
      bool onEdge = (x == 0) || (x == levelSize - 1) || (y == 0) || (y == levelSize - 1);
      bool isEven = (x % 2 == 0) && (y % 2 == 0);

      if (onEdge)
        level[y][x] = HARD_WALL;
      else
        level[y][x] = (random(3) > 1) ? SOFT_WALL : NONE;
    }

  if (level[playerY][playerX] == SOFT_WALL)       level[playerY][playerX] = NONE;
  if (level[playerY + 1][playerX] == SOFT_WALL)   level[playerY + 1][playerX] = NONE;
  if (level[playerY - 1][playerX] == SOFT_WALL)   level[playerY - 1][playerX] = NONE;
  if (level[playerY][playerX + 1] == SOFT_WALL)   level[playerY][playerX + 1] = NONE;
  if (level[playerY][playerX - 1] == SOFT_WALL)   level[playerY][playerX - 1] = NONE;

  wallsGenerated = 0;
  wallsDestroyed = 0;

  for(int y = 0; y < levelSize; y++)
    for(int x = 0; x < levelSize; x++)
      if (level[y][x] == SOFT_WALL)
        wallsGenerated++;
}

void playerUpdate()
{
  if (millis() - lastPlayerUpdate > playerUpdateTime)
  {
    lastPlayerUpdate = millis();

    int offX = joyX;
    int offY = joyY;

    if (level[playerY][playerX + offX] == NONE)
      playerX += offX;

    if (level[playerY + offY][playerX] == NONE)
      playerY += offY;

    if (level[playerY][playerX] == FIRE)
    {
      gameOver = true;
      return;
    }
  }

  if(!bombDown && digitalRead(sw) == LOW)
  {
    lastPlaceTime = millis();

    bombDown = true;
    bombX = playerX;
    bombY = playerY;
  }
}

void bombUpdate()
{
  if(bombDown && (millis() - lastPlaceTime > fuseTime))
  {
    bombDown = false;
    
    level[bombY][bombX] = FIRE;

    if (level[bombY + 1][bombX] != HARD_WALL)
    {
      if (level[bombY + 1][bombX] == SOFT_WALL)
      {
        wallsDestroyed++;
        triggerDrawUpdate = true;
      }

      level[bombY + 1][bombX] = FIRE;
    }

    if (level[bombY - 1][bombX] != HARD_WALL)
    {
      if (level[bombY - 1][bombX] == SOFT_WALL)
      {
        wallsDestroyed++;
        triggerDrawUpdate = true;
      }

      level[bombY - 1][bombX] = FIRE;
    }

    if (level[bombY][bombX + 1] != HARD_WALL)
    {
      if (level[bombY][bombX + 1] == SOFT_WALL)
      {
        wallsDestroyed++;
        triggerDrawUpdate = true;
      }

      level[bombY][bombX + 1] = FIRE;
    }

    if (level[bombY][bombX - 1] != HARD_WALL)
    {
      if (level[bombY][bombX - 1] == SOFT_WALL)
      {
        wallsDestroyed++;
        triggerDrawUpdate = true;
      }

      level[bombY][bombX - 1] = FIRE;
    }
  }
}

void clearFire()
{
  if (millis() - lastPlaceTime > fuseTime + fireTime)
    for (int y = 0; y < levelSize; y++)
      for (int x = 0; x < levelSize; x++)
        if (level[y][x] == FIRE)
          level[y][x] = NONE;
}

void blink(unsigned long& lastTime, unsigned long blinkTime, bool& blink)
{
  if (millis() - lastTime > blinkTime)
  {
    lastTime = millis();
    blink = !blink;
  }
}

void reset()
{
  gameOver = false;

  playerX = 1;
  playerY = 1;

  lastPlayerUpdate = 0;
  lastPlayerBlink = 0;
  lastBombBlink = 0;
  lastFireBlink = 0;
  lastPlaceTime = 0;
  lastReset = millis();

  generateLevel();

  for(int y = 0; y < matrixSize; y++)
    for(int x = 0; x < matrixSize; x++)
    {
      lc.setLed(0, y, x, level[y][x]);
      delay(10);
    }

  lcd.clear();

  triggerDrawUpdate = true;
}

void doGameOverEffect()
{
  for (int y = 0; y < matrixSize; y++)
    for (int x = 0; x < matrixSize; x++)
    {
      lc.setLed(0, y, x, false);
      delay(10);
    }

  for (int i = 0; i < sizeof(gameOverBitmap) - matrixSize; i++)
  {
    for (int x = i; x < i + matrixSize; x++)
      for (int y = 0; y < matrixSize; y++)
        lc.setLed(0, y, x - i, (gameOverBitmap[x] >> y) & 1);

    delay(50);
  }
}

void draw()
{
  blink(lastPlayerBlink, playerBlinkTime, playerBlink);
  blink(lastBombBlink, bombBlinkTime, bombBlink);
  blink(lastFireBlink, fireBlinkTime, fireBlink);

  int drawOffX = playerX - matrixSize / 2;
  int drawOffY = playerY - matrixSize / 2;
  int diff = levelSize - matrixSize;

  if (drawOffX < 0)
    drawOffX = 0;
  else if (drawOffX > diff)
    drawOffX = diff;

  if (drawOffY < 0)
    drawOffY = 0;
  else if (drawOffY > diff)
    drawOffY = diff;

  for(int y = 0; y < matrixSize; y++)
    for(int x = 0; x < matrixSize; x++)
    {
      uint8_t lx = x + drawOffX;
      uint8_t ly = y + drawOffY;

      if (lx == playerX && ly == playerY)
        lc.setLed(0, y, x, playerBlink);
      else if(bombDown && lx == bombX && ly == bombY)
        lc.setLed(0, y, x, bombBlink);
      else
        lc.setLed(0, y, x, (level[ly][lx] == FIRE) ? fireBlink : level[ly][lx]);
    }
}

void gameLoop() 
{
  if (!gameOver)
  {
    playerUpdate();
    bombUpdate();
    clearFire();
    draw();

    if (triggerDrawUpdate || millis() - lastDrawUpdate > 1000)
    {
      lastDrawUpdate = millis();
      triggerDrawUpdate = false;

      unsigned long totalSeconds = (millis() - lastReset) / 1000;
      unsigned long minutes = totalSeconds / 60;
      unsigned long seconds = totalSeconds % 60;

      lcd.setCursor(0, 0);
      lcd.print("Time: ");

      lcd.print(minutes);
      lcd.print(":");

      if (seconds < 10)
        lcd.print("0");

      lcd.print(seconds);
      
      lcd.setCursor(0, 1);
      lcd.print("Walls: ");

      lcd.print(wallsGenerated - wallsDestroyed);
      lcd.print("/");
      lcd.print(wallsGenerated);
    }
  }
  else
  {
    doGameOverEffect();
    
    menuIndex = 0;
    viewIndex = 0;
    state = GAME_OVER;
    drawGameOver();
  }
}
