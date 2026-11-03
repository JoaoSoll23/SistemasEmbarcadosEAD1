//Genius - Arduino Mega 2560

#include <Arduino.h>
#include <avr/interrupt.h>

// ---------------- PINOS ----------------

const uint8_t pinosLeds[4] = {2, 3, 4, 5};

const uint8_t pinosSegmentosNivel[7] = {
  22, 23, 24, 25, 26, 27, 28
};

const uint8_t pinosSegmentosTempo[7] = {
  30, 31, 32, 33, 34, 35, 36
};

const uint8_t pinoBuzzer = 7;

// ---------------- ESTADOS DO JOGO ----------------

enum EstadoDoJogo : uint8_t {
  esperaInicio,
  mostraSequencia,
  esperaJogador,
  gameover
};

volatile uint8_t botaoPressionado = 255;
volatile bool reinicioSolicitado = false;
volatile uint8_t segundosRestantes = 0;
volatile bool tempoEsgotado = false;
volatile EstadoDoJogo estado = esperaInicio;

uint8_t sequencia[40];
uint8_t tamanhoSequencia = 1;
uint8_t indiceJogador = 0;

volatile uint8_t nivelAtual = 0;

uint8_t indiceExibicao = 0;
uint8_t ledExibido = 255;

uint32_t proximaMudancaSequencia = 0;
uint32_t ultimaEntradaAceitaMs = 0;
uint32_t ultimoReinicioMs = 0;

// Controle do buzzer sem tone()
uint32_t pararBipEm = 0;

// Controle da animação de Game Over
uint32_t inicioAnimacaoFimDeJogo = 0;
uint32_t proximaPiscadaLeds = 0;
bool estadoPiscadaLeds = false;

// ---------------- DISPLAY DE 7 SEGMENTOS ----------------

// Bits correspondentes a A, B, C, D, E, F, G.
const uint8_t mapaDigitos[10] = {
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

void configurarSegmentos(const uint8_t pinos[7], uint8_t valor) {
  if (valor > 9) {
    valor = 0;
  }

  uint8_t padrao = mapaDigitos[valor];

  for (uint8_t i = 0; i < 7; i++) {
    digitalWrite(
      pinos[i],
      // LÓGICA INVERTIDA PARA ÂNODO COMUM
      (padrao & (1 << i)) ? LOW : HIGH 
    );
  }
}

// ---------------- LEDS ----------------

void apagarTodosLeds() {
  for (uint8_t i = 0; i < 4; i++) {
    digitalWrite(pinosLeds[i], LOW);
  }
}

// ---------------- BUZZER ----------------

void tocarBip(uint16_t frequencia, uint16_t duracaoMs) {
  (void)frequencia;

  digitalWrite(pinoBuzzer, HIGH);
  pararBipEm = millis() + duracaoMs;
}

void pararBip() {
  digitalWrite(pinoBuzzer, LOW);
  pararBipEm = 0;
}

// ---------------- FIM DE JOGO ----------------

void acionarFimDeJogo() {
  estado = gameover;
  segundosRestantes = 0;
  
  // Prepara variáveis para piscar LEDs por 3 segundos sem delay()
  inicioAnimacaoFimDeJogo = millis();
  proximaPiscadaLeds = millis();
  estadoPiscadaLeds = false;
  
  // Dispara um bip longo (1.5 segundos) de erro
  tocarBip(200, 1500); 
}

// ---------------- INICIAR JOGO ----------------

void iniciarJogo() {
  apagarTodosLeds();
  pararBip();

  nivelAtual = 1;
  tamanhoSequencia = 1;
  indiceJogador = 0;

  sequencia[0] = random(0, 4);

  estado = mostraSequencia;

  indiceExibicao = 0;
  ledExibido = 255;

  proximaMudancaSequencia = millis();

  segundosRestantes = 0;
  tempoEsgotado = false;

  tocarBip(880, 80);
}

// ---------------- INTERRUPCOES DOS BOTOES ----------------

ISR(INT0_vect) { botaoPressionado = 0; }
ISR(INT1_vect) { botaoPressionado = 1; }
ISR(INT2_vect) { botaoPressionado = 2; }
ISR(INT3_vect) { botaoPressionado = 3; }

ISR(PCINT0_vect) {
  if (!(PINB & _BV(PB0))) {
    reinicioSolicitado = true;
  }
}

// ---------------- TIMER 1 ----------------

ISR(TIMER1_COMPA_vect) {
  if (estado == esperaJogador && segundosRestantes > 0) {
    segundosRestantes--;
    if (segundosRestantes == 0) {
      tempoEsgotado = true;
    }
  }
}

// ---------------- TIMER 3 ----------------

ISR(TIMER3_COMPA_vect) {
  configurarSegmentos(pinosSegmentosNivel, nivelAtual % 10);
  configurarSegmentos(pinosSegmentosTempo, segundosRestantes % 10);
}

// ---------------- CONFIGURACAO DAS INTERRUPCOES ----------------

void configurarInterrupcoes() {
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

void configurarTimer1() {
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

void configurarTimer3() {
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
    pinMode(pinosLeds[i], OUTPUT);
  }

  for (uint8_t i = 0; i < 7; i++) {
    pinMode(pinosSegmentosNivel[i], OUTPUT);
    pinMode(pinosSegmentosTempo[i], OUTPUT);
  }

  pinMode(pinoBuzzer, OUTPUT);
  pinMode(53, INPUT_PULLUP);

  apagarTodosLeds();
  pararBip();

  configurarSegmentos(pinosSegmentosNivel, 0);
  configurarSegmentos(pinosSegmentosTempo, 0);

  randomSeed(analogRead(A0));

  configurarInterrupcoes();
  configurarTimer1();
  configurarTimer3();
}

// ---------------- LOOP PRINCIPAL ----------------

void loop() {
  uint32_t agora = millis();

  // Encerra o som (e apaga os LEDs de feedback do clique) quando o tempo termina.
  if (pararBipEm != 0 && (int32_t)(agora - pararBipEm) >= 0) {
    pararBip();
    // Apaga caso seja o flash rápido de quando o jogador aperta o botão
    if (estado == esperaJogador) apagarTodosLeds(); 
  }

  // Inicia ou reinicia o jogo.
  if (reinicioSolicitado) {
    noInterrupts();
    reinicioSolicitado = false;
    interrupts();

    // Filtro anti-ruído (debounce) de 500ms para o Restart físico
    if (agora - ultimoReinicioMs > 500) {
      ultimoReinicioMs = agora;
      iniciarJogo();
    }
  }

  // Verifica se o jogador esgotou o tempo.
  if (tempoEsgotado) {
    noInterrupts();
    tempoEsgotado = false;
    interrupts();
    acionarFimDeJogo();
  }

  // ---------------- ANIMAÇÃO DE GAME OVER ----------------
  if (estado == gameover) {
    if ((agora - inicioAnimacaoFimDeJogo) < 3000) {
      if ((int32_t)(agora - proximaPiscadaLeds) >= 0) {
        estadoPiscadaLeds = !estadoPiscadaLeds; // Inverte o estado
        for (uint8_t i = 0; i < 4; i++) {
          digitalWrite(pinosLeds[i], estadoPiscadaLeds ? HIGH : LOW);
        }
        proximaPiscadaLeds = agora + 250; // Pisca a cada 250ms
      }
    } else {
      // Garante que os LEDs fiquem apagados após os 3 segundos
      if (estadoPiscadaLeds) {
        apagarTodosLeds();
        estadoPiscadaLeds = false;
      }
    }
  }

  // ---------------- MOSTRAR SEQUENCIA ----------------

  if (estado == mostraSequencia && (int32_t)(agora - proximaMudancaSequencia) >= 0) {
    
    // APAGA AS LUZES DE PARABÉNS LOGO ANTES DE COMEÇAR A NOVA SEQUÊNCIA
    if (indiceExibicao == 0 && ledExibido == 255) apagarTodosLeds();

    if (ledExibido != 255) {
      digitalWrite(pinosLeds[ledExibido], LOW);
      ledExibido = 255;
      proximaMudancaSequencia = agora + 250;
    } else if (indiceExibicao < tamanhoSequencia) {
      ledExibido = sequencia[indiceExibicao];
      digitalWrite(pinosLeds[ledExibido], HIGH);
      tocarBip(440 + ledExibido * 110, 130);
      indiceExibicao++;
      proximaMudancaSequencia = agora + 500;
    } else {
      pararBip();
      apagarTodosLeds();
      estado = esperaJogador;
      indiceJogador = 0;
      segundosRestantes = 5 + tamanhoSequencia;
      tempoEsgotado = false;
    }
  }

  // ---------------- LER BOTAO PRESSIONADO ----------------

  uint8_t botao = 255;

  noInterrupts();
  botao = botaoPressionado;
  botaoPressionado = 255;
  interrupts();

  // ---------------- VALIDAR RESPOSTA ----------------

  if (estado == esperaJogador && botao < 4) {
    
    // Debounce simples de software de 120ms para a resposta do jogador
    if (agora - ultimaEntradaAceitaMs > 120) {
      ultimaEntradaAceitaMs = agora;

      if (botao == sequencia[indiceJogador]) {
        // Resposta correta: Acende o LED correspondente e bipa
        digitalWrite(pinosLeds[botao], HIGH);
        indiceJogador++;
        tocarBip(600 + botao * 100, 70);

        if (indiceJogador >= tamanhoSequencia) {
          // O jogador completou a sequencia (ACERTOU TUDO!)
          nivelAtual++;

          // --- FEEDBACK DE PARABÉNS ---
          for (uint8_t i = 0; i < 4; i++) {
            digitalWrite(pinosLeds[i], HIGH);
          }
          tocarBip(1200, 300);
          // -----------------------------

          if (tamanhoSequencia < sizeof(sequencia)) {
            sequencia[tamanhoSequencia++] = random(0, 4);
          }
          
          estado = mostraSequencia;
          indiceExibicao = 0;
          ledExibido = 255;
          proximaMudancaSequencia = agora + 800; // Tempo do "flash" de vitória antes de reiniciar
          segundosRestantes = 0;
        }
      } else {
        acionarFimDeJogo();
      }
    }
  }
}
