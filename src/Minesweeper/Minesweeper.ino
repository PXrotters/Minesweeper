#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// ——————————————————————————————————————————————
// 1. DEFINITII PINI
// ——————————————————————————————————————————————
#define TFT_CS_PIN    10
#define TFT_DC_PIN     8
#define TFT_RST_PIN    9

#define JOY_X_PIN     A0
#define JOY_Y_PIN     A1
#define JOY_SW_PIN     2   // Buton SW al joystick-ului (active-LOW)

#define BTN_FLAG       4   // Buton steag (active-HIGH)
#define BTN_RST        6   // Buton reset / confirm nume (active-HIGH)

#define BUZZER         5   // Buzzer pentru sunete

// ——————————————————————————————————————————————
// 2. OBIECT TFT (ST7735 pe SPI)
// ——————————————————————————————————————————————
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN);

// ——————————————————————————————————————————————
// 3. PARAMETRI JOC MINESWEEPER
// ——————————————————————————————————————————————
const uint8_t COLS = 8, ROWS = 8, BOMBS = 10;
uint8_t  board[ROWS][COLS];
bool     hidden[ROWS][COLS];            // true = acoperit
bool     flagged[ROWS][COLS];           // true = marcat cu steag
uint8_t  cursorRow = 0, cursorCol = 0;  // pozitia curenta a cursorului pe grid 

char     playerName[9]   = "        ";  // pana la 8 caractere + '\0'
uint8_t  nameLength      = 0;           // cate litere au fost selectate
bool     enteringName = true;           // esti in ecranul de introducere nume

bool     headerDrawn = false;           // ca sa nu redesenam tot de fiecare data 

unsigned long gameStartTime = 0;        // momentul inceperii jocului
const unsigned long GAME_TIME = 5UL * 60UL * 1000UL;  // Timpul jocului = 5 minute
uint16_t cellsUncovered = 0;            // cate celule descoperite

// ——————————————————————————————————————————————
// 4. DEBOUNCE PENTRU BUTOANE
// ——————————————————————————————————————————————
// Detectare apasare active-LOW (pull-down extern)
bool isButtonPressedActiveLow(uint8_t pin) {
  if (digitalRead(pin) == LOW) {
    delay(50);
    if (digitalRead(pin) == LOW) {
      while (digitalRead(pin) == LOW) { }
      return true;
    }
  }
  return false;
}

// Detectare apasare active-HIGH (pull-down extern)
bool isButtonPressedActiveHigh(uint8_t pin) {
  if (digitalRead(pin) == HIGH) {
    delay(50);
    if (digitalRead(pin) == HIGH) {
      while (digitalRead(pin) == HIGH) { }
      return true;
    }
  }
  return false;
}

// ——————————————————————————————————————————————
// 5. SUNETE & BOARD
// ——————————————————————————————————————————————
// Sir de tonuri la bomba
void playBombSequence() {
  // 3 tonuri descendente, legate
  for (int freq = 120; freq >= 60; freq -= 20) {
    tone(BUZZER, freq, 100);
    delay(120);
  }

  tone(BUZZER, 300, 300);
  delay(320);
  tone(BUZZER, 450, 200);
  delay(220);

  // Eco rapid (2 ecouri scurte, mai inalte)
  tone(BUZZER, 600, 80);
  delay(100);
  tone(BUZZER, 700, 60);
  delay(80);
  
  // Final, cadere in liniste
  noTone(BUZZER);
  delay(100);
}

// Sir de tonuri la castig
void playWinSequence() {
  tone(BUZZER, 523, 150); delay(180);
  tone(BUZZER, 659, 150); delay(180);
  tone(BUZZER, 784, 150); delay(180);

  tone(BUZZER, 1047, 200); delay(240);

  tone(BUZZER, 784, 100);  delay(120);
  tone(BUZZER, 659, 100);  delay(120);
  tone(BUZZER, 523, 100);  delay(120);

  tone(BUZZER, 440, 300);
  delay(320);
  noTone(BUZZER);
}

// Aseaza bombe si calculeaza valorile vecinilor
void generateBoard() {
  // Initializare
  for(int r = 0; r < ROWS; r++) {
    for(int c = 0; c < COLS; c++){
      board[r][c] = 0;
      hidden[r][c] = true;
      flagged[r][c] = false;
    }
  }

  // Plasam bombe
  uint8_t placed = 0;
  while(placed < BOMBS) {
    uint8_t r = random(0, ROWS), c = random(0, COLS);
    if(board[r][c] != 9){
      board[r][c] = 9;
      placed++;
    }
  }

  // Calculam vecinii
  for(int r = 0; r < ROWS; r++) {
    for(int c = 0; c < COLS; c++) {
      if(board[r][c] == 9) continue;
      uint8_t count = 0;
      for(int dr = -1; dr <= 1; dr++) {
        for(int dc = -1; dc <= 1; dc++){
          int nr = r + dr, nc = c + dc;
          if(nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS && board[nr][nc] == 9) count++;
        }
      }
      board[r][c] = count;
    }
  }
}

// Descopera recursiv (flood fill) pentru celulele cu valoare = 0
void flood(int r, int c) {
  if(r < 0 || r >= ROWS || c < 0 || c >= COLS) return;
  if(!hidden[r][c] || flagged[r][c]) return;
  hidden[r][c] = false;
  cellsUncovered++;
  if(board[r][c] == 0) {
    for(int dr = -1; dr <= 1; dr++) {
      for(int dc = -1; dc <= 1; dc++){
        flood(r + dr, c + dc);
      }
    }
  }
}

bool checkWin(){
  return cellsUncovered + BOMBS == ROWS * COLS;
}

// ——————————————————————————————————————————————
// 6. DESENAREA TABLEI SI A CURSORULUI
// ——————————————————————————————————————————————
void drawGrid() {
  const uint8_t cellW = 128 / COLS;  // Latimea celulei 
  const uint8_t cellH = 128 / ROWS;  // Inaltimea celulei
  for(int r = 0; r < ROWS; r++) {
    for(int c = 0; c < COLS; c++) {
      uint16_t x = c * cellW, y = 32 + r * cellH;  // Calculam coordonatele stanga sus al celulei, adaugand 32 pixeli pe verticala pentru a sare de header
      if(!hidden[r][c]) {
        tft.fillRect(x, y, cellW, cellH, ST77XX_WHITE);  // Daca celula nu e ascunsa o umlpe cu alb
        if(board[r][c] > 0 && board[r][c] < 9) {  // Daca celula are un numar de vecini (1-8), afiseaza acel numar in negru, centrat aprox in celula
          tft.setCursor(x + cellW / 3, y + cellH / 4);
          tft.setTextSize(1);
          tft.setTextColor(ST77XX_BLACK);
          tft.print(board[r][c]);
        }
        // Daca celula contine o bomba, generam bomba
        if(board[r][c] == 9) {
          int cx = x + cellW / 2;
          int cy = y + cellH / 2;
          int R  = min(cellW, cellH) / 3;    // Raza, putin mai mica decat 1/3 din dimensiunea celulei
          drawBomb(cx, cy, R);
        }
     } else {
       tft.fillRect(x, y, cellW, cellH, ST77XX_BLUE);  // Daca celula e ascunsa o umple cu albastru
       if(flagged[r][c]) {  // Daca e marcata cu steag, afisam litera F
         drawFlag(x, y, cellW, cellH);
       }
     }
    tft.drawRect(x, y, cellW, cellH, ST77XX_BLACK);  // Trasam conturul negru al celulei
    }
  }
  tft.drawRect(cursorCol * (128 / COLS), 32 + cursorRow * ((160 - 32) / ROWS), 128 / COLS, (160 - 32) / ROWS, ST77XX_GREEN);  // Trasanm un dreptunghi verde in jurul celulei indicate de cursorRow, cursorCol
}

// Actualizeaza doar valoarea numerica a scorului, stergand intai zona veche, apoi printand noul scor
void updateScore() {
  // stergem zona unde apare numarul
  tft.fillRect(2 + 6 * 6, 14, 24, 8, ST77XX_BLACK);
  tft.setCursor(2 + 6 * 6, 14);  // pozitionam cursorul
  tft.print(cellsUncovered);  // printam scorul actual
}

// Actualizeaza timer-ul in header
void updateTimer() {
  // stergem zona unde apare timer-ul
  tft.fillRect(80, 2, 44, 8, ST77XX_BLACK);
  // calculam secundele ramase  
  unsigned long elapsed = millis() - gameStartTime;
  unsigned long left = (GAME_TIME > elapsed ? (GAME_TIME - elapsed) / 1000 : 0);
  char buf[6];
  sprintf(buf, "%02lu:%02lu", left / 60, left % 60);
  // pozitionam cursorul si printam timer-ul
  tft.setCursor(80,2);
  tft.print(buf);
}

// ——————————————————————————————————————————————
// 8. MENIU DE START
// ——————————————————————————————————————————————
bool menuInit = false; 
uint8_t prevR = 0, prevC = 0;                             // pozitia literelor desenate anterior
const uint8_t LCOLS = 7, LROWS = 4;                       // dimensiune tabela litere
const uint8_t lw=128 / LCOLS, lh = (160 - 48) / LROWS;    // latimea celulei in meniul de litere
const uint8_t MENU_HEADER_SIZE = 1;                       // font pentru textul "Select Name"
const uint8_t MENU_LETTER_Y    = 12;                      // Y-ul de start pentru litere
const uint16_t BACKGND_MENU    = tft.color565(0,100,0);   // culoarea de background pentru meniul de intrare si de selectare a numelui (verde inchis)
const uint16_t BABY_BLUE       = tft.color565(173, 216, 230);

// Deseneaza ecranul de selectare nume: header + grila 7 x 4 litere
void drawLetterGrid() {
  tft.fillScreen(BACKGND_MENU);
  // header Select Name
  tft.setTextSize(MENU_HEADER_SIZE);
  tft.setTextColor(ST77XX_BLUE);
  tft.setCursor(0, 0);
  tft.print("Select Name");
  // grila
  tft.setTextSize(1);
  tft.setTextColor(BABY_BLUE);
  for(uint8_t r = 0; r < LROWS; r++){
    for(uint8_t c = 0; c < LCOLS; c++){
      uint8_t idx = r * LCOLS + c;
      char ch = idx < 26 ? 'A' + idx : ' ';
      tft.setCursor(c * lw + lw / 3, MENU_LETTER_Y + r * lh + lh / 3);
      tft.print(ch);
    }
  }
  // Initial nu exista litera anterioara
  prevR = prevC = 0;
}

// Reincarca complet zona de header (nume, scor si timer)
void drawHeader() {
  tft.fillRect(0, 0, 128, 32, ST77XX_BLACK);  // umple cu negru intreaga banda de sus
  tft.setTextSize(1);
  tft.setTextColor(BABY_BLUE);

  // Numele jucatorului
  tft.setCursor(2, 2);
  tft.print(playerName);

  // Scorul curent
  tft.setCursor(2, 14);
  tft.print("Score:");
  tft.print(cellsUncovered);

  // Timer-ul (format MM:SS)
  // calculeaza timpul scurs si timpul ramas
  unsigned long elapsed = millis() - gameStartTime;
  unsigned long left = (GAME_TIME > elapsed ? (GAME_TIME - elapsed) / 1000 : 0);

  // formateaza in buffer “MM:SS”
  char buf[6];
  sprintf(buf, "%02lu:%02lu", left / 60, left % 60);
  tft.setCursor(80, 2);
  tft.print(buf);
}

// ——————————————————————————————————————————————
// 9. DESENAREA ELEMENTELOR DE JOC
// ——————————————————————————————————————————————

// Desenam o bomba cu 8 tete, plasata la (cx,cy) cu raza R
void drawBomb(int cx, int cy, int R) {
  // corpul
  tft.fillCircle(cx, cy, R, ST77XX_BLACK);
  tft.drawCircle(cx, cy, R, ST77XX_WHITE);

  // Spike-urile bombei
  for (int i = 0; i < 8; i++) {
    float ang = i * (PI / 4.0);
    int x1 = cx + cos(ang) * (R - 2);
    int y1 = cy + sin(ang) * (R - 2);
    int x2 = cx + cos(ang) * (R + 4);
    int y2 = cy + sin(ang) * (R + 4);
    tft.drawLine(x1, y1, x2, y2, ST77XX_WHITE);
  }

  // Ochiul fitilului
  tft.fillCircle(cx, cy - R/2, R/4, ST77XX_RED);
  tft.drawFastHLine(cx - R/2, cy - R/2, R/2, ST77XX_YELLOW);
}

// Deseneaza un stegulet rosu in interiorul celulei definite de (x, y, width, height)
void drawFlag(uint16_t x, uint16_t y, uint8_t width, uint8_t height) {
  // 1) tija (linie neagra)
  uint16_t poleX = x + width / 4;
  uint16_t poleY1 = y + height / 4;
  uint16_t poleY2 = y + 3 * height / 4;
  tft.drawLine(poleX, poleY1, poleX, poleY2, ST77XX_BLACK);

  // 2) varful steagului (triunghi rosu)
  int16_t tx1 = poleX;
  int16_t ty1 = poleY1;
  int16_t tx2 = poleX + width/2;
  int16_t ty2 = poleY1 + height/8;
  int16_t tx3 = poleX;
  int16_t ty3 = poleY1 + height/4;
  tft.fillTriangle(tx1, ty1, tx2, ty2, tx3, ty3, ST77XX_RED);

  // 3) bordura usoara a triunghiului (linii albe)
  tft.drawTriangle(tx1, ty1, tx2, ty2, tx3, ty3, ST77XX_WHITE);
}

// Deseneaza 2 tarnacoape incrucisate
void drawCrossedPickaxes(int cx, int cy, int size) {
  // dimensiuni relative
  int handleLen = size * 12;    // lungime maner
  int handleW = size * 3;     // grosime maner
  int headLen = size * 6;     // cat „iese” capul metalic
  int headW = size * 4;     // latimea metalului

  // culori
  uint16_t colHandle = tft.color565(139,69,19); // maro inchis
  uint16_t colMetal  = tft.color565(200,200,200); // gri metalic
  uint16_t colEdge   = ST77XX_BLACK;
  uint16_t colHighlight = ST77XX_WHITE;

  // unghiuri pentru cele doua tarnacoape
  float angs[2] = { -PI/4, -3*PI/4 };

  for (int k = 0; k < 2; k++) {
    float ang = angs[k];

    // --- desenam manerul ---
    // capete
    int x0 = cx + cos(ang) * (handleLen/2);
    int y0 = cy + sin(ang) * (handleLen/2);
    int x1 = cx - cos(ang) * (handleLen/2);
    int y1 = cy - sin(ang) * (handleLen/2);

    // desenam un dreptunghi rotit pentru maner
    // offset perpendicular
    int dx = -sin(ang) * (handleW/2);
    int dy =  cos(ang) * (handleW/2);
    // cele 4 colturi
    int hx0 = x0 + dx, hy0 = y0 + dy;
    int hx1 = x0 - dx, hy1 = y0 - dy;
    int hx2 = x1 - dx, hy2 = y1 - dy;
    int hx3 = x1 + dx, hy3 = y1 + dy;
    // umple si contur
    tft.fillTriangle(hx0, hy0, hx1, hy1, hx2, hy2, colHandle);
    tft.fillTriangle(hx0, hy0, hx2, hy2, hx3, hy3, colHandle);
    tft.drawTriangle(hx0, hy0, hx1, hy1, hx2, hy2, colEdge);
    tft.drawLine(hx0, hy0, hx3, hy3, colEdge);
    tft.drawLine(hx1, hy1, hx2, hy2, colEdge);

    // --- desenam capul metalic ---
    // punctul de pornire la capatul manerului
    int mx = cx + cos(ang) * (handleLen/2);
    int my = cy + sin(ang) * (handleLen/2);
    // baza trapezului metalic (latime headW)
    float angPerp = ang + PI/2;
    int bx0 = mx + cos(angPerp) * (headW/2);
    int by0 = my + sin(angPerp) * (headW/2);
    int bx1 = mx - cos(angPerp) * (headW/2);
    int by1 = my - sin(angPerp) * (headW/2);
    // varful metalic
    int tx = mx + cos(ang) * headLen;
    int ty = my + sin(ang) * headLen;
    // umple trapezul
    tft.fillTriangle(bx0, by0, bx1, by1, tx, ty, colMetal);
    // contur
    tft.drawTriangle(bx0, by0, bx1, by1, tx, ty, colEdge);

    // highlight pe o fateta
    // un mic segment alb intre bx0,by0 si punctul de varf
    int hx = mx + cos(ang + 0.1) * (headLen * 0.7);
    int hy = my + sin(ang + 0.1) * (headLen * 0.7);
    tft.drawLine(bx0, by0, hx, hy, colHighlight);
  }
}

// ——————————————————————————————————————————————
// 10. SETUP & LOOP
// ——————————————————————————————————————————————

// Configurare initiala: pini, TFT
void setup() {
  // ——————————————————————————————————————————————
  // Configurare pini de intrare/iesire
  // ——————————————————————————————————————————————
  pinMode(JOY_SW_PIN, INPUT_PULLUP);  // buton joystick (activ LOW, pull-up intern)
  pinMode(BTN_FLAG, INPUT);           // buton steag (activ HIGH, pull-down extern)
  pinMode(BTN_RST, INPUT);            // buton reset/select name (activ HIGH, pull-down extern)
  pinMode(BUZZER, OUTPUT);            // buzzer pentru semnale sonore

  // ——————————————————————————————————————————————
  // Initializare afisaj TFT
  // ——————————————————————————————————————————————
  tft.initR(INITR_BLACKTAB);      // configurare driver ST7735
  tft.fillScreen(ST77XX_BLACK);   // curata ecranul (culoare neagra)
}

void loop() {
  static bool splashShown = false;

    // Splash‐screen „MINESWEEPER” la pornire
  if (!splashShown) {

    tft.fillScreen(BACKGND_MENU);
    tft.setTextSize(2);
    tft.setTextColor(BABY_BLUE);
    tft.setCursor(5, 50);
    tft.print("GOOD LUCK!");
    drawCrossedPickaxes(32, 120, 3);
    drawCrossedPickaxes(96, 120, 3);
    splashShown = true;

    // Asteapta pana se apasa oricare buton
    while (!isButtonPressedActiveLow(JOY_SW_PIN) && !isButtonPressedActiveHigh(BTN_FLAG) && !isButtonPressedActiveHigh(BTN_RST)) { }
    // Treci in meniul de selectare a numelui
    enteringName = true;
    menuInit = false;
    nameLength = 0;
    memset(playerName, ' ', 8);
    playerName[8] = '\0';
    tft.fillScreen(BACKGND_MENU);
    return;
  }

  if (enteringName) {
    // --- MENIU DE START ---
    if (!menuInit) {
      drawLetterGrid();  // Desenam tabela de litere
      menuInit = true;
    }

    // Redesenam celula precedenta: doar fundal + litera
    {
      // Suprascriem acea celula cu fundal negru, ca sa stergem conturul verde anterior
      uint16_t bx = prevC * lw;
      uint16_t by = MENU_LETTER_Y + prevR * lh;
      // fundal negru
      tft.fillRect(bx, by, lw, lh, BACKGND_MENU);
      // Retragem caracterul 'A' + idx in culoarea alba, pentru a reface grila literei in celula respectiva
      uint8_t idx = prevR * LCOLS + prevC;
      if (idx < 26) {
        tft.setTextSize(1);
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(bx + lw / 3, by + lh / 3);
        tft.print(char('A' + idx));
      }
    }

    // Navigare in grid-ul de litere
    static uint8_t nameR = 0, nameC = 0;
    int vx = analogRead(JOY_X_PIN), vy = analogRead(JOY_Y_PIN);    // Citim pozitia joystick-ului
    if      (vx < 300 && nameC > 0)       { nameC--; delay(150); } // Punem un mic delay pentru debouncing si pentru a nu schimba de prea multe ori in bucla 
    else if (vx > 700 && nameC < LCOLS-1) { nameC++; delay(150); }
    if      (vy < 300 && nameR > 0)       { nameR--; delay(150); }
    else if (vy > 700 && nameR < LROWS-1) { nameR++; delay(150); }

    // Desenam chenarul verde nou
    tft.drawRect(nameC*lw, MENU_LETTER_Y + nameR*lh, lw, lh, ST77XX_GREEN);
    // Salvam pozitiile curente pentru a sti unde stergem conturul data viitoare
    prevR = nameR;
    prevC = nameC;

    // Bara de jos cu numele curent
    tft.fillRect(0, 160 - 24, 128, 24, ST77XX_BLUE);
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(10, 160 - 20);
    tft.print(playerName);

    // Confirm litera
    if (isButtonPressedActiveLow(JOY_SW_PIN) && nameLength < 8) {
      uint8_t idx = nameR * LCOLS + nameC;
      if (idx < 26) {
        playerName[nameLength++] = 'A' + idx;
        playerName[nameLength] = '\0';
      }
    }

    // Confirm final nume
    if (isButtonPressedActiveHigh(BTN_RST) && nameLength > 0) {
      enteringName = false;
      menuInit = false;
      headerDrawn = false;
      generateBoard();
      gameStartTime = millis();
      cellsUncovered = 0;
      cursorRow = cursorCol = 0;
      tft.fillScreen(BACKGND_MENU);
      return;
    }

  } else {
    // --- ECRAN JOC ---
    drawHeader();
    drawGrid();

    // Reset -> Reintrare in meniul de selectare a numelui
    if (isButtonPressedActiveHigh(BTN_RST)) {
      enteringName = true;
      nameLength = 0;
      memset(playerName,' ',8);
      playerName[8] = '\0';
      headerDrawn = false;
      return;
    }

    // Navigare cursor pe grid
    int x = analogRead(JOY_X_PIN), y = analogRead(JOY_Y_PIN);
    if      (x < 300 && cursorCol > 0)       { cursorCol--; delay(150); }
    else if (x > 700 && cursorCol < COLS-1)  { cursorCol++; delay(150); }
    if      (y < 300 && cursorRow > 0)       { cursorRow--; delay(150); }
    else if (y > 700 && cursorRow < ROWS-1)  { cursorRow++; delay(150); }

    // Pune/ia steag
    if (isButtonPressedActiveHigh(BTN_FLAG) && hidden[cursorRow][cursorCol]) {
      flagged[cursorRow][cursorCol] = !flagged[cursorRow][cursorCol];
      delay(200);
    }

    // Descoperire celula
    if (isButtonPressedActiveLow(JOY_SW_PIN) && hidden[cursorRow][cursorCol] && !flagged[cursorRow][cursorCol]) {
      if (board[cursorRow][cursorCol] == 9) {
        // Bomba → game over
        hidden[cursorRow][cursorCol] = false;
        updateScore(); updateTimer(); drawGrid();
        playBombSequence();
        delay(1000);
        enteringName = true;
        tft.fillScreen(ST77XX_RED);
        tft.setTextSize(2);
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(10,70);
        tft.print("GAME OVER");
        delay(2000);
        nameLength = 0;
        memset(playerName,' ',8);
        playerName[8] = '\0';
        return;
      }

      // Flood sau uncover normal
      if (board[cursorRow][cursorCol] == 0) {
        flood(cursorRow,cursorCol);
      } else {
        hidden[cursorRow][cursorCol] = false;
        cellsUncovered++;
      }
      delay(200);

      // Verifica castig
      if (checkWin()) {
        playWinSequence();
        tft.fillScreen(ST77XX_GREEN);
        tft.setTextSize(2);
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(10,70);
        tft.print("YOU WIN!");
        delay(2000);
        enteringName = true;
        nameLength = 0;
        memset(playerName,' ',8);
        playerName[8] = '\0';
        return;
      }
    }

    // TIME UP
    if (millis() - gameStartTime >= GAME_TIME) {
      playBombSequence();
      tft.fillScreen(ST77XX_ORANGE);
      tft.setTextSize(2);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(10,70);
      tft.print("TIME UP!");
      delay(2000);
      enteringName = true;
      nameLength = 0;
      memset(playerName,' ',8);
      playerName[8] = '\0';
      return;
    }
  }
}
