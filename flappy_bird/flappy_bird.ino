
#include <Badge2020_Accelerometer.h>
#include <Badge2020_TFT.h>

Badge2020_TFT tft;

#define TOUCH_0 27
#define TOUCH_1 14
#define TOUCH_2 13

#define UPDATE_TIME_MS 16

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define COLOR_BACKGROUND CYAN

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240

#define DELTA_TIME 0.016
#define GRAVITY 0.3
#define FLAP_VELOCITY -5.0

enum GameMode {
  Intro = 1,
  Playing,
  GameOver
};

struct GameState {
  GameMode current_mode;
  int pillar_pos;
  int gap_pos;
  int score;
  int high_score = 0;
  float bird_x, bird_y, bird_vel, bird_acc;
  // Optimization: we only draw certain parts on the first frame, and don't draw things on next frames.
  bool first_frame;
  bool button_was_pressed;

  GameState();
  void reset();
};

GameState::GameState() {
  current_mode = GameMode::Intro;
  high_score = 0;
  button_was_pressed = false;
}

void GameState::reset() {
  score = 0;
  pillar_pos = SCREEN_WIDTH;
  gap_pos = random(20, 120);
  bird_x = 10.0;
  bird_y = 0.0;
  bird_vel = 0.0;
  bird_acc = GRAVITY;
  first_frame = true;
  button_was_pressed = false;
}

GameState game_state;

long nextDrawLoopRunTime;

#define BUTTON_HIGH 1
#define BUTTON_LOW 0

int lastButtonState = BUTTON_LOW;
int currentButtonState = BUTTON_LOW;

void setup() {
  Serial.begin(115200);

  tft.init(SCREEN_WIDTH, SCREEN_HEIGHT);
  tft.setRotation(2);
  game_state.reset();
}

void drawBorder () {
  uint16_t width = tft.width() - 1;
  uint16_t height = tft.height() - 1;
  uint8_t border = 10;

  tft.fillScreen(BLUE);
  tft.fillRect(border, border, (width - border * 2), (height - border * 2), WHITE);
}

void drawFlappy(int x, int y) {
  // Upper & lower body
  tft.fillRect(x + 2, y + 8, 2, 10, BLACK);
  tft.fillRect(x + 4, y + 6, 2, 2, BLACK);
  tft.fillRect(x + 6, y + 4, 2, 2, BLACK);
  tft.fillRect(x + 8, y + 2, 4, 2, BLACK);
  tft.fillRect(x + 12, y, 12, 2, BLACK);
  tft.fillRect(x + 24, y + 2, 2, 2, BLACK);
  tft.fillRect(x + 26, y + 4, 2, 2, BLACK);
  tft.fillRect(x + 28, y + 6, 2, 6, BLACK);
  tft.fillRect(x + 10, y + 22, 10, 2, BLACK);
  tft.fillRect(x + 4, y + 18, 2, 2, BLACK);
  tft.fillRect(x + 6, y + 20, 4, 2, BLACK);

  // Body fill
  tft.fillRect(x + 12, y + 2, 6, 2, YELLOW);
  tft.fillRect(x + 8, y + 4, 8, 2, YELLOW);
  tft.fillRect(x + 6, y + 6, 10, 2, YELLOW);
  tft.fillRect(x + 4, y + 8, 12, 2, YELLOW);
  tft.fillRect(x + 4, y + 10, 14, 2, YELLOW);
  tft.fillRect(x + 4, y + 12, 16, 2, YELLOW);
  tft.fillRect(x + 4, y + 14, 14, 2, YELLOW);
  tft.fillRect(x + 4, y + 16, 12, 2, YELLOW);
  tft.fillRect(x + 6, y + 18, 12, 2, YELLOW);
  tft.fillRect(x + 10, y + 20, 10, 2, YELLOW);

  // Eye
  tft.fillRect(x + 18, y + 2, 2, 2, BLACK);
  tft.fillRect(x + 16, y + 4, 2, 6, BLACK);
  tft.fillRect(x + 18, y + 10, 2, 2, BLACK);
  tft.fillRect(x + 18, y + 4, 2, 6, WHITE);
  tft.fillRect(x + 20, y + 2, 4, 10, WHITE);
  tft.fillRect(x + 24, y + 4, 2, 8, WHITE);
  tft.fillRect(x + 26, y + 6, 2, 6, WHITE);
  tft.fillRect(x + 24, y + 6, 2, 4, BLACK);

  // Beak
  tft.fillRect(x + 20, y + 12, 12, 2, BLACK);
  tft.fillRect(x + 18, y + 14, 2, 2, BLACK);
  tft.fillRect(x + 20, y + 14, 12, 2, RED);
  tft.fillRect(x + 32, y + 14, 2, 2, BLACK);
  tft.fillRect(x + 16, y + 16, 2, 2, BLACK);
  tft.fillRect(x + 18, y + 16, 2, 2, RED);
  tft.fillRect(x + 20, y + 16, 12, 2, BLACK);
  tft.fillRect(x + 18, y + 18, 2, 2, BLACK);
  tft.fillRect(x + 20, y + 18, 10, 2, RED);
  tft.fillRect(x + 30, y + 18, 2, 2, BLACK);
  tft.fillRect(x + 20, y + 20, 10, 2, BLACK);
}

void clearScreen() {
  tft.fillScreen(COLOR_BACKGROUND);
}

void drawGround() {
  int ty = 230;
  for (int tx = 0; tx <= 300; tx += 20) {
    tft.fillTriangle(tx, ty, tx + 10, ty, tx, ty + 10, GREEN);
    tft.fillTriangle(tx + 10, ty + 10, tx + 10, ty, tx, ty + 10, YELLOW);
    tft.fillTriangle(tx + 10, ty, tx + 20, ty, tx + 10, ty + 10, YELLOW);
    tft.fillTriangle(tx + 20, ty + 10, tx + 20, ty, tx + 10, ty + 10, GREEN);
  }
}

void drawPillar(int x, int gap) {
  tft.fillRect(x + 2, 2, 46, gap - 4, GREEN);
  tft.fillRect(x + 2, gap + 92, 46, 136 - gap, GREEN);

  tft.drawRect(x, 0, 50, gap, BLACK);
  tft.drawRect(x + 1, 1, 48, gap - 2, BLACK);
  tft.drawRect(x, gap + 90, 50, 140 - gap, BLACK);
  tft.drawRect(x + 1, gap + 91 , 48, 138 - gap, BLACK);
}

void clearPillar(int x, int gap) {
  tft.fillRect(x + 45, 0, 5, gap, COLOR_BACKGROUND);
  tft.fillRect(x + 45, gap + 90, 5, 140 - gap, COLOR_BACKGROUND);
}

void clearFlappy(int x, int y) {
  tft.fillRect(x, y, 34, 24, COLOR_BACKGROUND);
}

bool checkCollision() {
  // Collision with ground
  if (game_state.bird_y > 206) {
    return true;
  }

  // Collision with pillar
  if (game_state.bird_x + 34 > game_state.pillar_pos && game_state.bird_x < game_state.pillar_pos + 50) {
    if (game_state.bird_y < game_state.gap_pos || game_state.bird_y + 24 > game_state.gap_pos + 90) {
      return true;
    }
  }

  return false;
}


void drawIntro() {
  if (game_state.first_frame) {
    tft.fillScreen(BLACK);
    tft.setCursor (40, 50);
    tft.setTextSize (3);
    tft.setTextColor(GREEN);
    tft.println("Flappy Bird");
    tft.setCursor (85, 85);
    tft.setTextSize (2);
    tft.setTextColor(WHITE);
    tft.println("by");
    tft.setCursor (50, 120);
    tft.setTextSize (2);
    tft.setTextColor(RED);
    tft.println("ERLIN & FREEE");

    tft.setCursor (50, 200);
    tft.setTextSize (1);
    tft.setTextColor(WHITE);
    tft.println("Druk op 2!!!");
    game_state.first_frame = false;
  }
}

void drawGameOver() {
  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.setCursor(50, 75);
  tft.print("Game Over!");

  tft.setCursor(50, 125);
  tft.print("Score:");
  tft.setCursor(170, 125);
  tft.print(game_state.score);

  tft.setCursor(50, 145);
  tft.print("High Score:");
  tft.setCursor(170, 145);
  tft.print(game_state.high_score);
}

void eraseLevel() {
  clearPillar(game_state.pillar_pos, game_state.gap_pos);
  clearFlappy(game_state.bird_x, game_state.bird_y);
}

void drawLevel() {
  if (game_state.first_frame) {
    clearScreen();
    drawGround();
    game_state.first_frame = false;
  }
  drawPillar(game_state.pillar_pos, game_state.gap_pos);
  drawFlappy(game_state.bird_x, game_state.bird_y);
}

void buttonLogic() {
  currentButtonState = touchRead(TOUCH_2) > 0 ? BUTTON_HIGH : BUTTON_LOW;
  if (lastButtonState == BUTTON_HIGH && currentButtonState == BUTTON_LOW) {
    game_state.button_was_pressed = true;
    //Serial.println("button");
  }
  lastButtonState = currentButtonState;
}

void gameLogic(bool buttonPressed) {
  if (buttonPressed) {
    game_state.bird_vel = FLAP_VELOCITY;
  }

  game_state.bird_vel += game_state.bird_acc;
  game_state.bird_y += game_state.bird_vel;

  game_state.pillar_pos -= 2;
  if (game_state.pillar_pos == 0) {
    game_state.score += 1;
  } else if (game_state.pillar_pos < -50) {
    game_state.pillar_pos = 320;
    game_state.gap_pos = random(20, 120);
  }
}


void loop() {
  // Button logic
  buttonLogic();

  // Update logic
  if (millis() > nextDrawLoopRunTime) {
    // Drawing logic
    switch (game_state.current_mode) {
      case GameMode::Intro:
        drawIntro();
        if (game_state.button_was_pressed) {
          game_state.reset();
          game_state.current_mode = GameMode::Playing;
        }
        break;
      case GameMode::Playing:
        // Clearing the entire screen causes it to flicker. Instead, we erase the previous elements (bird + pillar), then draw the new ones.
        // We do this *before* executing the game logic, because the game logic will change the positions of the elements.
        eraseLevel();
        gameLogic(game_state.button_was_pressed);
        drawLevel();
        if (checkCollision()) {
          delay(100);
          game_state.current_mode = GameMode::GameOver;
          if (game_state.high_score < game_state.score) {
            game_state.high_score = game_state.score;
          }
        }
        break;
      case GameMode::GameOver:
        drawGameOver();
        if (game_state.button_was_pressed) {
          game_state.reset();
          game_state.current_mode = GameMode::Playing;
        }
        break;
    }

    game_state.button_was_pressed = false;
    nextDrawLoopRunTime += UPDATE_TIME_MS;
  }
}
