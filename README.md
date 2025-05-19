# 🧨 Minesweeper pe Arduino UNO R3

## Introducere

🧠 **Scopul proiectului**: Jucătorul explorează o matrice de celule, evitând bombele 💣 și marcându-le corect cu steaguri 🚩. Am pornit de la ideea jocului original de pe PC și l-am adaptat pe un sistem embedded pentru a-l face mai interactiv. Este un joc care antrenează logica și atenția, fiind în același timp o demonstrație practică a integrării hardware–software.

## Descriere generală

🎮 Proiectul constă în implementarea jocului **Minesweeper** pe un ecran LCD TFT ST7735S de 1.8” (128×160), controlat prin SPI de către un microcontroller **Arduino UNO R3**.  
👾 Jucătorul navighează o matrice 8×8 folosind un **joystick analogic**, iar cele **3 butoane fizice** sunt folosite pentru acțiuni precum descoperirea celulelor, marcarea steagurilor și resetarea jocului.  
🔊 Un **buzzer** oferă feedback auditiv la pierdere (bombă) sau câștig.

## Hardware Design

### 🔌 Bill of Materials

| Componentă                | Tip             | Cantitate | Preț / buc | Total   |
|--------------------------|------------------|-----------|------------|---------|
| Buton alb                | Buton            | 2         | 1,99 lei   | 3,98 lei |
| Buton albastru           | Buton            | 2         | 1,99 lei   | 3,98 lei |
| Joystick analogic        | Joystick         | 1         | 4,96 lei   | 4,96 lei |
| LCD TFT 1.8", 128×160    | Display SPI      | 1         | 54,99 lei  | 54,99 lei |
| Arduino UNO R3 + cablu   | Placă dezvoltare | 1         | 39,37 lei  | 39,37 lei |
| Buzzer Pasiv 5V          | Buzzer           | 1         | 1,40 lei   | 1,40 lei |
| Fire rigide              | Set              | 1         | 12,49 lei  | 12,49 lei |
| Fire tată-tată           | Set              | 4         | 2,85 lei   | 11,40 lei |
| Breadboard 400 puncte    | Breadboard       | 1         | 4,56 lei   | 4,56 lei |

**💰 Cost total: 137,13 lei**

### 🧩 Schema bloc



📷 Configurația fizică finală: `final.jpg`

## Software Design

🛠️ Proiectul a fost dezvoltat în **Arduino IDE** în limbajul **C/C++**.  
📺 Pentru grafica pe ecranul LCD ST7735S s-au utilizat:

- [`Adafruit_GFX.h`](https://github.com/adafruit/Adafruit-GFX-Library)
- [`Adafruit_ST7735.h`](https://github.com/adafruit/Adafruit-ST7735-Library)
- [`SPI.h`](https://www.arduino.cc/en/Reference/SPI)

### 🔄 Funcții implementate

| Funcție                  | Descriere |
|--------------------------|-----------|
| `isButtonPressedActiveLow()` | Verifică apăsare buton activ LOW cu debounce |
| `isButtonPressedActiveHigh()` | Verifică apăsare buton activ HIGH cu debounce |
| `playBombSequence()` | Sunet buzzer la pierdere |
| `playWinSequence()` | Sunet buzzer la câștig |
| `generateBoard()` | Plasare bombe și calcul vecini |
| `flood()` | Descoperire recursivă celule goale |
| `checkWin()` | Verificare condiție de câștig |
| `drawGrid()` | Redare completă matrice 8x8 pe LCD |
| `updateScore()` | Actualizare scor pe ecran |
| `updateTimer()` | Actualizare timer MM:SS |
| `drawLetterGrid()` | Ecran selectare nume (grilă litere) |
| `drawHeader()` | Afișare bandă sus (scor, timp) |
| `drawBomb()` | Desen bombă grafică |
| `drawFlag()` | Desen steag într-o celulă |
| `drawCrossedPickaxes()` | Desen târnăcoape încrucișate |

### 📐 Funcții grafice utilizate

| Funcție         | Descriere |
|------------------|-----------|
| `fillScreen()` | Curăță tot ecranul |
| `fillRect()` | Umple un dreptunghi |
| `drawRect()` | Desenează conturul unui dreptunghi |
| `fillCircle()` | Desenează cerc solid |
| `drawCircle()` | Contur cerc |
| `drawFastHLine()` | Linie orizontală rapidă |
| `drawLine()` | Linie oblică |
| `fillTriangle()` | Triunghi solid |
| `drawTriangle()` | Contur triunghi |
| `fillRoundRect()` | Dreptunghi cu colțuri rotunjite |
| `setCursor()` | Poziționează text |
| `setTextSize()` | Setează mărimea textului |
| `setTextColor()` | Setează culoarea textului |

## ✅ Concluzii

Proiectul Minesweeper pe Arduino a fost o experiență foarte reușită, combinând logica jocului cu grafică interactivă în timp real. Bucla de redare continuă mi-a reamintit de temele din cursul de grafică și m-a ajutat să înțeleg mai bine optimizările necesare pentru un sistem cu resurse limitate. Am lucrat eficient cu joystick-ul, butoane, GPIO, ADC și interfața SPI, dezvoltând atât partea hardware, cât și software. A fost un exercițiu excelent de integrare între componente și logică de joc.

## 📚 Bibliografie / Resurse

### Hardware

- [Arduino UNO R3 Datasheet](https://store.arduino.cc/products/arduino-uno-rev3)
- [ATmega328P Datasheet](https://www.microchip.com/en-us/product/ATmega328P)
- [TFT Display Guide](https://learn.adafruit.com/1-8-tft-display)
- [Joystick Guide](https://www.dfrobot.com/wiki/index.php/Analog_Joystick_Module_SKU:_DFR0061)
- [Button Guide](https://docs.arduino.cc/tutorials/generic/button)
- [Buzzer Guide](https://components101.com/buzzer)

### Software

- [Adafruit-GFX-Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [Adafruit ST7735 Library](https://github.com/adafruit/Adafruit-ST7735-Library)
- [Arduino SPI Library](https://www.arduino.cc/en/Reference/SPI)
