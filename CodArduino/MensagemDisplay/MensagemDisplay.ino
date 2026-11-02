
#include <avr/interrupt.h>

// Pinos do display de segmentos
const int segA = 2;
const int segB = 3;
const int segC = 4;
const int segD = 5;
const int segE = 6;
const int segF = 7;
const int segG = 8;

// Quantidade de posições que o display possui
const int totalCaracteres = 13;

int posicao = 0;

// Sinalização do timer
volatile bool atualizarMensagem = false;

// Botões de interrupção
const int botaoAvancar = 21;  // INT0
const int botaoVoltar = 20;   // INT1
const int botaoReset = 53;    // PCINT0

// Solicitações das interrupções
volatile bool solicitarAvanco = false;
volatile bool solicitarRetorno = false;
volatile bool solicitarReset = false;

// Potenciômetro
const int pinoPotenciometro = A0;

// Intervalos de funcionamento
const unsigned int tempoMinimo = 200;
const unsigned int tempoMaximo = 4000;

// Valor inicial de comparação do Timer1
uint16_t ultimoOCR1A = 15624;

// Função para exibir as letras e números são 13 porque há um - separando as palavras
//low = ligado/ high = desligado
void NumDisplay(int num) {

  switch (num) {

    case 0: 
      digitalWrite(segA, LOW);
      digitalWrite(segB, HIGH);
      digitalWrite(segC, LOW);
      digitalWrite(segD, LOW);
      digitalWrite(segE, HIGH);
      digitalWrite(segF, LOW);
      digitalWrite(segG, LOW);
      break;

    case 1:
      digitalWrite(segA, LOW);
      digitalWrite(segB, HIGH);
      digitalWrite(segC, HIGH);
      digitalWrite(segD, LOW);
      digitalWrite(segE, LOW);
      digitalWrite(segF, LOW);
      digitalWrite(segG, LOW);
      break;

    case 2:
      digitalWrite(segA, HIGH);
      digitalWrite(segB, HIGH);
      digitalWrite(segC, HIGH);
      digitalWrite(segD, HIGH);
      digitalWrite(segE, HIGH);
      digitalWrite(segF, HIGH);
      digitalWrite(segG, LOW);
      break;

    case 3:
      digitalWrite(segA, LOW);
      digitalWrite(segB, LOW);
      digitalWrite(segC, LOW);
      digitalWrite(segD, LOW);
      digitalWrite(segE, LOW);
      digitalWrite(segF, LOW);
      digitalWrite(segG, HIGH);
      break;

    case 4:
      digitalWrite(segA, HIGH);
      digitalWrite(segB, HIGH);
      digitalWrite(segC, LOW);
      digitalWrite(segD, HIGH);
      digitalWrite(segE, LOW);
      digitalWrite(segF, HIGH);
      digitalWrite(segG, LOW);
      break;

    case 5:
      digitalWrite(segA, HIGH);
      digitalWrite(segB, HIGH);
      digitalWrite(segC, HIGH);
      digitalWrite(segD, HIGH);
      digitalWrite(segE, HIGH);
      digitalWrite(segF, HIGH);
      digitalWrite(segG, LOW);
      break;

    case 6:
      digitalWrite(segA, LOW);
      digitalWrite(segB, LOW);
      digitalWrite(segC, LOW);
      digitalWrite(segD, HIGH);
      digitalWrite(segE, LOW);
      digitalWrite(segF, LOW);
      digitalWrite(segG, LOW);
      break;

    case 7:
      digitalWrite(segA, LOW);
      digitalWrite(segB, HIGH);
      digitalWrite(segC, HIGH);
      digitalWrite(segD, HIGH);
      digitalWrite(segE, LOW);
      digitalWrite(segF, LOW);
      digitalWrite(segG, HIGH);
      break;

    case 8:
      digitalWrite(segA, HIGH);
      digitalWrite(segB, HIGH);
      digitalWrite(segC, HIGH);
      digitalWrite(segD, HIGH);
      digitalWrite(segE, HIGH);
      digitalWrite(segF, HIGH);
      digitalWrite(segG, LOW);
      break;

    case 9:
      digitalWrite(segA, LOW);
      digitalWrite(segB, LOW);
      digitalWrite(segC, HIGH);
      digitalWrite(segD, LOW);
      digitalWrite(segE, LOW);
      digitalWrite(segF, HIGH);
      digitalWrite(segG, LOW);
      break;

    case 10:
      digitalWrite(segA, LOW);
      digitalWrite(segB, HIGH);
      digitalWrite(segC, LOW);
      digitalWrite(segD, LOW);
      digitalWrite(segE, HIGH);
      digitalWrite(segF, LOW);
      digitalWrite(segG, LOW);
      break;

    case 11:
      digitalWrite(segA, LOW);
      digitalWrite(segB, HIGH);
      digitalWrite(segC, LOW);
      digitalWrite(segD, LOW);
      digitalWrite(segE, LOW);
      digitalWrite(segF, LOW);
      digitalWrite(segG, LOW);
      break;

    case 12:
      digitalWrite(segA, LOW);
      digitalWrite(segB, LOW);
      digitalWrite(segC, LOW);
      digitalWrite(segD, LOW);
      digitalWrite(segE, LOW);
      digitalWrite(segF, LOW);
      digitalWrite(segG, HIGH);
      break;
  }
}

// Movimentação da mensagem
void moverPosicao(int deslocamento) {

  posicao += deslocamento;

  if (posicao >= totalCaracteres) {
    posicao = 0;
  }

  if (posicao < 0) {
    posicao = totalCaracteres - 1;
  }

  NumDisplay(posicao);
}

// INT0: avançar
  void interrupcaoAvancar() {
    solicitarAvanco = true;
  }

  // INT1: retornar
  void interrupcaoVoltar() {
    solicitarRetorno = true;
  }

  // PIN CHANGE: reiniciar
  ISR(PCINT0_vect) {

  // Verifica se D53 (PB0) está em LOW
  if (!(PINB & (1 << PB0))) {
    solicitarReset = true;
  }
}

  // Interrupção do TIMER1
  ISR(TIMER1_COMPA_vect) {
    atualizarMensagem = true;
  }

  // Configuração do TIMER1 - modo CTC
  // Calcula o OCR1A conforme o potenciômetro
  uint16_t calcularOCR1A(int leitura) {

    // Divide a faixa em intervalos de 50 ms
  unsigned long passos = map(leitura, 0, 1023, 0, (tempoMaximo - tempoMinimo) / 50);

  // Calcula o tempo desejado
  unsigned long tempoMs = tempoMinimo + (passos * 50UL);

  // Calcula a quantidade de contagens do Timer
  unsigned long contagens = ((F_CPU / 1024UL) * tempoMs) / 1000UL;

  // Retorna o valor de comparação
  return (uint16_t)(contagens - 1);
}

// Atualiza o período do Timer conforme o potenciômetro
void atualizarPeriodoTimer() {

  static unsigned long ultimaLeitura = 0;

  unsigned long agora = millis();

  // Realiza uma leitura a cada 50 ms
  if (agora - ultimaLeitura < 50) {
    return;
  }

  ultimaLeitura = agora;

  // Lê o potenciômetro
  int leitura = analogRead(pinoPotenciometro);

  // Calcula o novo valor de comparação
  uint16_t novoOCR1A = calcularOCR1A(leitura);

  // Só atualiza se o valor realmente mudou
  if (novoOCR1A == ultimoOCR1A) {
    return;
  }

  // Guarda o valor do prescaler atual
  const uint8_t mascaraPrescaler = (1 << CS12) | (1 << CS11) | (1 << CS10);

  noInterrupts();

  uint8_t prescalerAtual = TCCR1B & mascaraPrescaler;

  // Pausa temporariamente o Timer1
  TCCR1B &= ~mascaraPrescaler;

  // Verifica a contagem atual
  uint16_t contagemAtual = TCNT1;

  // Atualiza o valor de comparação
  OCR1A = novoOCR1A;

  // Se a contagem já ultrapassou o novo limite,
  // reinicia o contador para evitar uma espera excessiva
  if (contagemAtual >= novoOCR1A) {
    TCNT1 = 0;
  }

  // Retoma o Timer com o mesmo prescaler
  TCCR1B |= prescalerAtual;

  interrupts();

  // Atualiza o valor armazenado
  ultimoOCR1A = novoOCR1A;
}

void configurarTimer1() {

  cli();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  // Define o intervalo inicial calculado pelo potenciômetro
  OCR1A = ultimoOCR1A;

  // Limpa flag de comparação pendente
  TIFR1 = (1 << OCF1A);

  // Habilita interrupção por comparação A
  TIMSK1 |= (1 << OCIE1A);

  // Configura modo CTC
  TCCR1B |= (1 << WGM12);

  // Prescaler 1024
  TCCR1B |= (1 << CS12) | (1 << CS10);

  sei();
}

// SETUP

void setup() {

  // Configuração dos segmentos
  pinMode(segA, OUTPUT);
  pinMode(segB, OUTPUT);
  pinMode(segC, OUTPUT);
  pinMode(segD, OUTPUT);
  pinMode(segE, OUTPUT);
  pinMode(segF, OUTPUT);
  pinMode(segG, OUTPUT);

  // Configuração dos botões
  pinMode(botaoAvancar, INPUT_PULLUP);
  pinMode(botaoVoltar, INPUT_PULLUP);
  pinMode(botaoReset, INPUT_PULLUP);

  // Problema: ao iniciar o arduino, posição 0 (S) estava sendo pulada
  //Soluçao: registrador EIFR, existe um comportamento chamado Write One to Clear (W1C)
  //Significa que, diferentemente de uma variável comum, escrever 1 em determinado bit faz com que sua flag seja limpa. 
  //escrevendo 00000011 faz limpar INT0 e INT1  EIFR = (1 << INTF0) | (1 << INTF1);
  EIFR = (1 << INTF0) | (1 << INTF1);

  // Configuração das interrupções externas
  attachInterrupt(
    digitalPinToInterrupt(botaoAvancar),
    interrupcaoAvancar,
    FALLING
  );

  attachInterrupt(
    digitalPinToInterrupt(botaoVoltar),
    interrupcaoVoltar,
    FALLING
  );

  // Configuração do Pin Change
  PCMSK0 |= (1 << PCINT0);

  // Limpa flag pendente do grupo PCINT0
  PCIFR = (1 << PCIF0);

  // Habilita o grupo PCINT0
  PCICR |= (1 << PCIE0);

  // Configuração do potenciômetro
  pinMode(pinoPotenciometro, INPUT);

  // Calcula o período inicial
  ultimoOCR1A = calcularOCR1A(
    analogRead(pinoPotenciometro)
);

  // Apresenta o primeiro padrão
  NumDisplay(posicao);

  // Inicia o Timer1
  configurarTimer1();

}

// Loop principal
void loop() {

  // Atualiza a velocidade conforme o potenciômetro
  atualizarPeriodoTimer();

  bool eventoTimer;
  bool eventoAvancar;
  bool eventoVoltar;
  bool eventoReset;

  // Lê e limpa os eventos de maneira protegida
  noInterrupts();

  eventoTimer = atualizarMensagem;
  atualizarMensagem = false;

  eventoAvancar = solicitarAvanco;
  solicitarAvanco = false;

  eventoVoltar = solicitarRetorno;
  solicitarRetorno = false;

  eventoReset = solicitarReset;
  solicitarReset = false;

  interrupts();

  // Debounce dos botões
  static unsigned long ultimoAvanco = 0;
  static unsigned long ultimoRetorno = 0;
  static unsigned long ultimoReset = 0;

  static bool primeiroAvanco = true;
  static bool primeiroRetorno = true;
  static bool primeiroReset = true;

  unsigned long agora = millis();

  bool houveAcaoManual = false;

  // PIN CHANGE: reset

  if (eventoReset && (primeiroReset || agora - ultimoReset >= 60)) {

    primeiroReset = false;
    ultimoReset = agora;

    // Retorna ao primeiro caractere
    posicao = 0;

    // Atualiza o display
    NumDisplay(posicao);

    // Reinicia o intervalo do Timer
    noInterrupts();

    atualizarMensagem = false;
    solicitarAvanco = false;
    solicitarRetorno = false;

    TCNT1 = 0;

    // Limpa flag de comparação pendente
    TIFR1 = (1 << OCF1A);

    interrupts();

    // Prioridade para o reset
    return;
  }

  // INT0: avançar
  if (eventoAvancar && (primeiroAvanco || agora - ultimoAvanco >= 60)) {

    primeiroAvanco = false;
    ultimoAvanco = agora;

    moverPosicao(1);

    houveAcaoManual = true;
  }

  // INT1: retornar
  else if (eventoVoltar && (primeiroRetorno || agora - ultimoRetorno >= 60)) {

    primeiroRetorno = false;
    ultimoRetorno = agora;

    moverPosicao(-1);

    houveAcaoManual = true;
  }

  // TIMER: avanço automático
  if (eventoTimer && !houveAcaoManual) {

    moverPosicao(1);
  }
}
