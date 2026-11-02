/*
  Genius / Simon - Arduino Mega 2560
  LEDs: D2, D3, D4, D5
  Botoes do jogo: D21, D20, D19, D18 (INT0 a INT3)
  Botao iniciar/reiniciar: D53 (PCINT0)
  Buzzer: D7
  Display do nivel: D22 a D28 (A a G)
  Display do tempo: D30 a D36 (A a G)
  Timer1: contagem regressiva de 1 segundo.
  Timer3: atualizacao dos displays.
  Sem delay() na logica principal.
*/

#include <Arduino.h>
#include <avr/interrupt.h>

// ---------------- PINOS ----------------

const uint8_t ledPins[4] = {2, 3, 4, 5};

const uint8_t segmentPinsLevel[7] = {
  22, 23, 24, 25, 26, 27, 28
};

const uint8_t segmentPinsTime[7] = {
  30, 31, 32, 33, 34, 35, 36
};

const uint8_t buzzerPin = 7;

// ---------------- ESTADOS DO JOGO ----------------

enum GameState : uint8_t {
  WAIT_START,
  SHOW_SEQUENCE,
  WAIT_INPUT,
  GAME_OVER
};

volatile uint8_t pressedButton = 255;
volatile bool restartRequested = false;
volatile uint8_t secondsLeft = 0;
volatile bool timeExpired = false;
volatile GameState state = WAIT_START;

uint8_t sequence[40];
uint8_t sequenceLength = 1;
uint8_t inputIndex = 0;

volatile uint8_t level = 0;

uint8_t showIndex = 0;
uint8_t shownLed = 255;

uint32_t nextSequenceChange = 0;
uint32_t lastAcceptedInputMs = 0;
uint32_t lastRestartMs = 0;

// Controle do buzzer sem tone()
uint32_t beepStopAt = 0;

// Controle da animação de Game Over
uint32_t gameOverAnimStart = 0;
uint32_t nextBlinkTime = 0;
bool ledBlinkState = false;

// ---------------- DISPLAY DE 7 SEGMENTOS ----------------

// Bits correspondentes a A, B, C, D, E, F, G.
const uint8_t digitMap[10] = {
  0b00111111, // 0
  0b00000110, // 1
  0b01011011, // 2
  0b01001111, // 3
  0b01100110, // 4
  0b01101101, // 5
  0b01111101, // 6
  0b00000111, // 7
  0b01111111, // 8
  0b01101111  // 9
};

void setSegments(const uint8_t pins[7], uint8_t value) {
  if (value > 9) {
    value = 0;
  }

  uint8_t pattern = digitMap[value];

  for (uint8_t i = 0; i < 7; i++) {
    digitalWrite(
      pins[i],
      // LÓGICA INVERTIDA PARA ÂNODO COMUM
      (pattern & (1 << i)) ? LOW : HIGH 
    );
  }
}

// ---------------- LEDS ----------------

void allLedsOff() {
  for (uint8_t i = 0; i < 4; i++) {
    digitalWrite(ledPins[i], LOW);
  }
}

// ---------------- BUZZER ----------------

void beep(uint16_t frequency, uint16_t durationMs) {
  (void)frequency;

  digitalWrite(buzzerPin, HIGH);
  beepStopAt = millis() + durationMs;
}

void stopBeep() {
  digitalWrite(buzzerPin, LOW);
  beepStopAt = 0;
}

// ---------------- FIM DE JOGO ----------------

void gameOver() {
  state = GAME_OVER;
  secondsLeft = 0;
  
  // Prepara variáveis para piscar LEDs por 3 segundos sem delay()
  gameOverAnimStart = millis();
  nextBlinkTime = millis();
  ledBlinkState = false;
  
  // Dispara um bip longo (1.5 segundos) de erro
  beep(200, 1500); 
}

// ---------------- INICIAR JOGO ----------------

void startGame() {
  allLedsOff();
  stopBeep();

  level = 1;
  sequenceLength = 1;
  inputIndex = 0;

  sequence[0] = random(0, 4);

  state = SHOW_SEQUENCE;

  showIndex = 0;
  shownLed = 255;

  nextSequenceChange = millis();

  secondsLeft = 0;
  timeExpired = false;

  beep(880, 80);
}

// ---------------- INTERRUPCOES DOS BOTOES ----------------

ISR(INT0_vect) { pressedButton = 0; }
ISR(INT1_vect) { pressedButton = 1; }
ISR(INT2_vect) { pressedButton = 2; }
ISR(INT3_vect) { pressedButton = 3; }

ISR(PCINT0_vect) {
  if (!(PINB & _BV(PB0))) {
    restartRequested = true;
  }
}

// ---------------- TIMER 1 ----------------

ISR(TIMER1_COMPA_vect) {
  if (state == WAIT_INPUT && secondsLeft > 0) {
    secondsLeft--;
    if (secondsLeft == 0) {
      timeExpired = true;
    }
  }
}

// ---------------- TIMER 3 ----------------

ISR(TIMER3_COMPA_vect) {
  setSegments(segmentPinsLevel, level % 10);
  setSegments(segmentPinsTime, secondsLeft % 10);
}

// ---------------- CONFIGURACAO DAS INTERRUPCOES ----------------

void setupInterrupts() {
  cli();

  DDRD &= ~0x0F;
  PORTD |= 0x0F;

  EICRA = (EICRA & 0x00) | 0xAA;
  EIFR = 0x0F;
  EIMSK |= 0x0F;

  DDRB &= ~_BV(DDB0);
  PORTB |= _BV(PORTB0);

  PCICR |= _BV(PCIE0);
  PCMSK0 |= _BV(PCINT0);
  PCIFR |= _BV(PCIF0);

  sei();
}

// ---------------- CONFIGURACAO DOS TIMERS ----------------

void setupTimer1() {
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 15624;
  TCCR1B |= _BV(WGM12);
  TCCR1B |= _BV(CS12) | _BV(CS10);
  TIMSK1 |= _BV(OCIE1A);
  sei();
}

void setupTimer3() {
  cli();
  TCCR3A = 0;
  TCCR3B = 0;
  TCNT3 = 0;
  OCR3A = 249;
  TCCR3B |= _BV(WGM32);
  TCCR3B |= _BV(CS31) | _BV(CS30);
  TIMSK3 |= _BV(OCIE3A);
  sei();
}

// ---------------- SETUP ----------------

void setup() {
  for (uint8_t i = 0; i < 4; i++) {
    pinMode(ledPins[i], OUTPUT);
  }

  for (uint8_t i = 0; i < 7; i++) {
    pinMode(segmentPinsLevel[i], OUTPUT);
    pinMode(segmentPinsTime[i], OUTPUT);
  }

  pinMode(buzzerPin, OUTPUT);
  pinMode(53, INPUT_PULLUP);

  allLedsOff();
  stopBeep();

  setSegments(segmentPinsLevel, 0);
  setSegments(segmentPinsTime, 0);

  randomSeed(analogRead(A0));

  setupInterrupts();
  setupTimer1();
  setupTimer3();
}

// ---------------- LOOP PRINCIPAL ----------------

void loop() {
  uint32_t now = millis();

  // Encerra o som (e apaga os LEDs de feedback do clique) quando o tempo termina.
  if (beepStopAt != 0 && (int32_t)(now - beepStopAt) >= 0) {
    stopBeep();
    // Apaga caso seja o flash rápido de quando o jogador aperta o botão
    if (state == WAIT_INPUT) allLedsOff(); 
  }

  // Inicia ou reinicia o jogo.
  if (restartRequested) {
    noInterrupts();
    restartRequested = false;
    interrupts();

    // Filtro anti-ruído (debounce) de 500ms para o Restart físico
    if (now - lastRestartMs > 500) {
      lastRestartMs = now;
      startGame();
    }
  }

  // Verifica se o jogador esgotou o tempo.
  if (timeExpired) {
    noInterrupts();
    timeExpired = false;
    interrupts();
    gameOver();
  }

  // ---------------- ANIMAÇÃO DE GAME OVER ----------------
  if (state == GAME_OVER) {
    if ((now - gameOverAnimStart) < 3000) {
      if ((int32_t)(now - nextBlinkTime) >= 0) {
        ledBlinkState = !ledBlinkState; // Inverte o estado
        for (uint8_t i = 0; i < 4; i++) {
          digitalWrite(ledPins[i], ledBlinkState ? HIGH : LOW);
        }
        nextBlinkTime = now + 250; // Pisca a cada 250ms
      }
    } else {
      // Garante que os LEDs fiquem apagados após os 3 segundos
      if (ledBlinkState) {
        allLedsOff();
        ledBlinkState = false;
      }
    }
  }

  // ---------------- MOSTRAR SEQUENCIA ----------------

  if (state == SHOW_SEQUENCE && (int32_t)(now - nextSequenceChange) >= 0) {
    
    // APAGA AS LUZES DE PARABÉNS LOGO ANTES DE COMEÇAR A NOVA SEQUÊNCIA
    if (showIndex == 0 && shownLed == 255) allLedsOff();

    if (shownLed != 255) {
      digitalWrite(ledPins[shownLed], LOW);
      shownLed = 255;
      nextSequenceChange = now + 250;
    } else if (showIndex < sequenceLength) {
      shownLed = sequence[showIndex];
      digitalWrite(ledPins[shownLed], HIGH);
      beep(440 + shownLed * 110, 130);
      showIndex++;
      nextSequenceChange = now + 500;
    } else {
      stopBeep();
      allLedsOff();
      state = WAIT_INPUT;
      inputIndex = 0;
      secondsLeft = 5 + sequenceLength;
      timeExpired = false;
    }
  }

  // ---------------- LER BOTAO PRESSIONADO ----------------

  uint8_t button = 255;

  noInterrupts();
  button = pressedButton;
  pressedButton = 255;
  interrupts();

  // ---------------- VALIDAR RESPOSTA ----------------

  if (state == WAIT_INPUT && button < 4) {
    
    // Debounce simples de software de 120ms para a resposta do jogador
    if (now - lastAcceptedInputMs > 120) {
      lastAcceptedInputMs = now;

      if (button == sequence[inputIndex]) {
        // Resposta correta: Acende o LED correspondente e bipa
        digitalWrite(ledPins[button], HIGH);
        inputIndex++;
        beep(600 + button * 100, 70);

        if (inputIndex >= sequenceLength) {
          // O jogador completou a sequencia (ACERTOU TUDO!)
          level++;

          // --- FEEDBACK DE PARABÉNS ---
          for (uint8_t i = 0; i < 4; i++) {
            digitalWrite(ledPins[i], HIGH);
          }
          beep(1200, 300);
          // -----------------------------

          if (sequenceLength < sizeof(sequence)) {
            sequence[sequenceLength++] = random(0, 4);
          }
          
          state = SHOW_SEQUENCE;
          showIndex = 0;
          shownLed = 255;
          nextSequenceChange = now + 800; // Tempo do "flash" de vitória antes de reiniciar
          secondsLeft = 0;
        }
      } else {
        gameOver();
      }
    }
  }
}
