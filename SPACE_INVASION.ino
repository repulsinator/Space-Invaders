#include <LCDWIKI_GUI.h>
#include <LCDWIKI_SPI.h>

// ===== Color Definitions =====
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// ===== TFT Display Setup =====
#define MODEL ILI9341
#define CS    10    
#define CD    9
#define RST   8
#define MOSI  11
#define MISO  12
#define SCK   13
#define LED   A0

LCDWIKI_SPI mylcd(MODEL, CS, CD, MISO, MOSI, RST, SCK, LED);

// ===== Joystick and Buzzer Pins =====
#define JOY_X A3
#define JOY_SW 5
#define BUZZER 6


// ===== Screen Size =====
const int SCREEN_WIDTH = 320;
const int SCREEN_HEIGHT = 240;

// ===== Player Properties =====
int playerX = SCREEN_WIDTH / 2 - 10;
const int playerY = SCREEN_HEIGHT - 20;
const int playerW = 20;
const int playerH = 10;

// ===== Bullet Properties =====
bool bulletActive = false;
int bulletX, bulletY;
const int bulletW = 4;
const int bulletH = 8;

// ===== Enemy Properties =====
const int ENEMY_ROWS = 2;
const int ENEMY_COLS = 6;
const int enemyW = 18;
const int enemyH = 10;
int enemyX[ENEMY_ROWS][ENEMY_COLS];
int enemyY[ENEMY_ROWS][ENEMY_COLS];
bool enemyAlive[ENEMY_ROWS][ENEMY_COLS];
int enemyDX = 2; // movement speed

// ===== Game Timing =====
unsigned long previousMillis = 0;
const long updateInterval = 30; // 30ms update rate

// ====== Functions ======
void setup() {
  mylcd.Init_LCD();
  mylcd.Fill_Screen(BLACK);
  Serial.begin(9600);
  pinMode(JOY_SW, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(LED, HIGH); // turn on backlight

  // Initialize enemies
  for (int row = 0; row < ENEMY_ROWS; row++) {
    for (int col = 0; col < ENEMY_COLS; col++) {
      enemyX[row][col] = 30 + col * 40;
      enemyY[row][col] = 30 + row * 30;
      enemyAlive[row][col] = true;
    }
  }
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= updateInterval) {
    previousMillis = currentMillis;
    updateGame();
    renderGame();
  }
}

void updateGame() {
  int joystickX = analogRead(JOY_X);
  int deadZone = 50;    // Center deadzone
  int centerValue = 512;
  int moveSpeedMax = 7; // Maximum move speed
  Serial.println(analogRead(JOY_X));

  int deviation = joystickX - centerValue;

  // Move player with proportional speed
  if (abs(deviation) > deadZone) {
    int moveAmount = map(deviation, -512, 512, -moveSpeedMax, moveSpeedMax);
    playerX += moveAmount;

    // Keep player within screen
    if (playerX < 0) playerX = 0;
    if (playerX > SCREEN_WIDTH - playerW) playerX = SCREEN_WIDTH - playerW;
  }
  
  // Fire bullet
  if (digitalRead(JOY_SW) == LOW && !bulletActive) {
    bulletActive = true;
    bulletX = playerX + playerW / 2 - bulletW / 2;
    bulletY = playerY;
    tone(BUZZER, 1000, 100); // short beep
  }

  // Update bullet
  if (bulletActive) {
    bulletY -= 20; // FASTER BULLET SPEED
    if (bulletY < 0) {
      bulletActive = false;
    }
  }

  // Move enemies
  bool needToDescend = false;
  for (int row = 0; row < ENEMY_ROWS; row++) {
    for (int col = 0; col < ENEMY_COLS; col++) {
      if (enemyAlive[row][col]) {
        enemyX[row][col] += enemyDX;
        if (enemyX[row][col] <= 0 || enemyX[row][col] >= SCREEN_WIDTH - enemyW) {
          needToDescend = true;
        }
      }
    }
  }
  
  if (needToDescend) {
    enemyDX = -enemyDX;
    for (int row = 0; row < ENEMY_ROWS; row++) {
      for (int col = 0; col < ENEMY_COLS; col++) {
        enemyY[row][col] += 10;
      }
    }
  }

  // Check bullet collision with enemies
  if (bulletActive) {
    for (int row = 0; row < ENEMY_ROWS; row++) {
        for (int col = 0; col < ENEMY_COLS; col++) {
            if (enemyAlive[row][col]) {
                bool horizontalOverlap = (bulletX + bulletW > enemyX[row][col]) && (bulletX < enemyX[row][col] + enemyW);
                bool verticalPassed = (bulletY + bulletH <= enemyY[row][col] + enemyH) && (bulletY + bulletH >= enemyY[row][col]);

                if (horizontalOverlap && verticalPassed) {
                    enemyAlive[row][col] = false;
                    bulletActive = false;
                    tone(BUZZER, 2000, 100); // hit sound
                    break;
                }
            }
        }
    }
}
}

void renderGame() {
  static int oldPlayerX = -1;
  static int oldBulletX = -1, oldBulletY = -1;
  static int oldEnemyX[ENEMY_ROWS][ENEMY_COLS], oldEnemyY[ENEMY_ROWS][ENEMY_COLS];

  // Erase previous player
  if (oldPlayerX != playerX) {
    mylcd.Fill_Rect(oldPlayerX, playerY, playerW, playerH, BLACK);
  }

  // Draw player
  mylcd.Fill_Rect(playerX, playerY, playerW, playerH, BLUE);
  oldPlayerX = playerX;

  // Erase previous bullet
  if (oldBulletX != bulletX || oldBulletY != bulletY) {
    mylcd.Fill_Rect(oldBulletX, oldBulletY, bulletW, bulletH, BLACK);
  }

  // Draw bullet
  if (bulletActive) {
    mylcd.Fill_Rect(bulletX, bulletY, bulletW, bulletH, WHITE);
    oldBulletX = bulletX;
    oldBulletY = bulletY;
  }

  // Draw enemies
  for (int row = 0; row < ENEMY_ROWS; row++) {
    for (int col = 0; col < ENEMY_COLS; col++) {
      if (enemyAlive[row][col]) {
        if (oldEnemyX[row][col] != enemyX[row][col] || oldEnemyY[row][col] != enemyY[row][col]) {
          mylcd.Fill_Rect(oldEnemyX[row][col], oldEnemyY[row][col], enemyW, enemyH, BLACK);
        }
        mylcd.Fill_Rect(enemyX[row][col], enemyY[row][col], enemyW, enemyH, RED);
        oldEnemyX[row][col] = enemyX[row][col];
        oldEnemyY[row][col] = enemyY[row][col];
      } else {
        mylcd.Fill_Rect(oldEnemyX[row][col], oldEnemyY[row][col], enemyW, enemyH, BLACK);
      }
    }
  }
}
