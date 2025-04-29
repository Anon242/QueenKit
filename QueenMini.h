#define LENGHT_ARRAY_OUT 36 //  Длина сообщения
#define LENGHT_ARRAY_IN 64  //  Длина кольцевого массива (буфера)
#define ROUND_MASK                                                             \
  63 //  Маска для предотвращения выхода за пределы кольцевого массива
#define ID_MASK 31
#define ADC_MASK 15
#define SNAKE_LENGTH 35 //  Значения для постоянного контроля отставания чтения
#define SNAKE_LENGTHRPI                                                        \
  39                 //  Значения для постоянного контроля отставания чтения RPI
#define CRC8_CELL 1  //  Смещение для обнаружения контрольной суммы
#define ID_CELL 2    //  Смещение для обнаружения id устройства
#define DEVICE_ID 16 //  номер ID данного устройства
#define RPI 1
#define RAPTOR 0
#define DEL 1
#define ADD 0
#define WATCH_DOG 100

#define STEP_2048 0xFFFFF800UL //  Шаги в миллисекундах для таймер счетчика
#define STEP_1024 0xFFFFF400UL
#define STEP_128 0xFFFFFF80UL
#define STEP_8 0xFFFFFFF8UL
#define STEP_4 0xFFFFFFF4UL
#define STEP_2 0xFFFFFFF2UL

uint8_t dataRPI[32][32];    //  Массив данных, полученных от RASPBERRYPI
uint8_t dataRaptor[32][32]; //  Массив данных, полученных от внешних плат R4
                            //  (Raptor)

uint8_t telomereRPI[32];    //  Массив рейтинга данных отправляемых RPI
uint8_t telomereRaptor[32]; //  Массив сроков жизни данных отправляемых платам
                            //  R4 (Raptor)

uint8_t sendstring[40] = {0};
// 0     1      2
uint8_t tail = 0; // *    crc    id
uint8_t outRaptors[LENGHT_ARRAY_OUT] = {
    0x2A, 0x01, 0x57, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x25};
uint8_t roundBuffer[LENGHT_ARRAY_IN];
uint8_t crcBuff[34];

static volatile unsigned long t2 = 0, p2 = 0;
volatile int watchDog = WATCH_DOG;

uint8_t tailrpi = 0;
// public sta           0      1     2    3     4    5     6      7     8     9
// 10    11    12    13    14    15    16    17    18    19    20    21    22 23
// 24    25    26    27    28    29    30    31    32    33    34    35    36
//                     Q     B     R    SIZE  COUN   ID   TIME  CRC  |   RELAY
//                     |                                                    16
//                     PWM CHANELS |                        RESERVED BYTES
uint8_t inRPI[40] = {0x51, 0x42, 0x52, 0x20, 0x00, 0x10, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// public sta           0      1     2    3     4    5     6      7     8     9
// 10    11    12    13    14    15    16    17    18    19    20    21    22 23
// 24    25    26    27    28    29    30    31    32    33    34    35    36
//                     Q     B     A    SIZE  COUN   ID   TIME  CRC  |   DINS |
//                     ADINS |                        RESERVED BYTES
uint8_t outRPI[40] = {0x51, 0x42, 0x41, 0x20, 0x00, 0x10, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t roundBufferRPI[LENGHT_ARRAY_IN];

uint32_t in_state = 0;
uint32_t out_state = 0;
int stringSumm = 0;

int currID = 0;
uint32_t telomereRaptorMap = 0;
uint32_t telomereRPiMap = 0;
int adc = 0;
int counterADSC = 0;
uint8_t DINS1 = 0, DINS2 = 0;
uint32_t valueADC = 0;

const uint8_t crc8Table[256] = {
    0x0,  0x31, 0x62, 0x53, 0xc4, 0xf5, 0xa6, 0x97, 0xb9, 0x88, 0xdb, 0xea,
    0x7d, 0x4c, 0x1f, 0x2e, 0x43, 0x72, 0x21, 0x10, 0x87, 0xb6, 0xe5, 0xd4,
    0xfa, 0xcb, 0x98, 0xa9, 0x3e, 0xf,  0x5c, 0x6d, 0x86, 0xb7, 0xe4, 0xd5,
    0x42, 0x73, 0x20, 0x11, 0x3f, 0xe,  0x5d, 0x6c, 0xfb, 0xca, 0x99, 0xa8,
    0xc5, 0xf4, 0xa7, 0x96, 0x1,  0x30, 0x63, 0x52, 0x7c, 0x4d, 0x1e, 0x2f,
    0xb8, 0x89, 0xda, 0xeb, 0x3d, 0xc,  0x5f, 0x6e, 0xf9, 0xc8, 0x9b, 0xaa,
    0x84, 0xb5, 0xe6, 0xd7, 0x40, 0x71, 0x22, 0x13, 0x7e, 0x4f, 0x1c, 0x2d,
    0xba, 0x8b, 0xd8, 0xe9, 0xc7, 0xf6, 0xa5, 0x94, 0x3,  0x32, 0x61, 0x50,
    0xbb, 0x8a, 0xd9, 0xe8, 0x7f, 0x4e, 0x1d, 0x2c, 0x2,  0x33, 0x60, 0x51,
    0xc6, 0xf7, 0xa4, 0x95, 0xf8, 0xc9, 0x9a, 0xab, 0x3c, 0xd,  0x5e, 0x6f,
    0x41, 0x70, 0x23, 0x12, 0x85, 0xb4, 0xe7, 0xd6, 0x7a, 0x4b, 0x18, 0x29,
    0xbe, 0x8f, 0xdc, 0xed, 0xc3, 0xf2, 0xa1, 0x90, 0x7,  0x36, 0x65, 0x54,
    0x39, 0x8,  0x5b, 0x6a, 0xfd, 0xcc, 0x9f, 0xae, 0x80, 0xb1, 0xe2, 0xd3,
    0x44, 0x75, 0x26, 0x17, 0xfc, 0xcd, 0x9e, 0xaf, 0x38, 0x9,  0x5a, 0x6b,
    0x45, 0x74, 0x27, 0x16, 0x81, 0xb0, 0xe3, 0xd2, 0xbf, 0x8e, 0xdd, 0xec,
    0x7b, 0x4a, 0x19, 0x28, 0x6,  0x37, 0x64, 0x55, 0xc2, 0xf3, 0xa0, 0x91,
    0x47, 0x76, 0x25, 0x14, 0x83, 0xb2, 0xe1, 0xd0, 0xfe, 0xcf, 0x9c, 0xad,
    0x3a, 0xb,  0x58, 0x69, 0x4,  0x35, 0x66, 0x57, 0xc0, 0xf1, 0xa2, 0x93,
    0xbd, 0x8c, 0xdf, 0xee, 0x79, 0x48, 0x1b, 0x2a, 0xc1, 0xf0, 0xa3, 0x92,
    0x5,  0x34, 0x67, 0x56, 0x78, 0x49, 0x1a, 0x2b, 0xbc, 0x8d, 0xde, 0xef,
    0x82, 0xb3, 0xe0, 0xd1, 0x46, 0x77, 0x24, 0x15, 0x3b, 0xa,  0x59, 0x68,
    0xff, 0xce, 0x9d, 0xac,
};

/**
 * @brief Получить полезную нагрузку
 *
 *  Получает uint32_t значение битов данных из шины
 *
 * @param arrPos С какой позиции получать биты
 * @param bits Сколько битов
 * @return uint32_t
 */
inline uint32_t getBits(uint32_t arrPos, uint32_t bits) {
  uint32_t byteIndex = arrPos >> 3;
  uint32_t bitIndex = arrPos & 7;

  uint64_t chunk;
  memcpy(&chunk, &dataRPI[16][byteIndex], sizeof(uint64_t));

  return (chunk >> bitIndex) & ((1ULL << bits) - 1);
}
/**
 * @brief Добавить данные в нагрузку шины
 *
 * @param bits Сколько битов
 * @param bytes Данные
 */
inline void setBits(uint32_t arrPos, uint32_t bits, uint32_t bytes) {
  uint32_t byteIndex = arrPos >> 3;
  uint32_t bitIndex = arrPos & 7;

  uint64_t chunk;
  memcpy(&chunk, &dataRaptor[16][byteIndex], sizeof(uint64_t));

  const uint64_t mask = ((1ULL << bits) - 1) << bitIndex;
  chunk = (chunk & ~mask) | ((bytes << bitIndex) & mask);

  memcpy(&dataRaptor[16][byteIndex], &chunk, sizeof(uint64_t));
}

uint16_t adcBuffer[6];

/**
 * @brief Метод для получения контрольной суммы
 * @warning Использует табличный метод, необходима заранее высчитанная
 * таблица значений с полиномом 0x31. Начальный crc строго 0xFF.
 * @param data массив данных
 * @param length длинна массива
 * @return uint8_t контрольная сумма
 */
inline uint8_t crc8(const uint8_t *data, uint8_t length) {
  uint8_t crc = 0xFF;
  for (size_t i = 0; i < length; i++) {
    crc = crc8Table[crc ^ data[i]];
  }
  return crc;
}

void setup() {
  startSetupPWM();
  startSetupADC(); //  Настройка АЦП

  Serial1.begin(250000);
  Serial2.begin(115200);


  telomereRPiMap |= ((uint32_t)1 << DEVICE_ID);
}
void startSetupPWM(){
  TCCR2A |= (1 << WGM20) | (1 << COM2B1);
  OCR2B = 0;
  TCCR5A |= (1 << WGM50) | (1 << COM5B1) | (1 << COM5A1) | (1 << COM5C1);
  OCR5A = 0;
  OCR5B = 0;
  OCR5C = 0;
}
void loop() {
  while (1) {
    priemrpi();
    priem();

    if (telomereRaptorMap)
      dogTimer(); // если серверу нужна хоть одна плата то запускаем таймер
  }
}

inline void dogTimer() {
  t2 = millis();
  if (((t2 & STEP_2) != (p2 & STEP_2))) {
    if (watchDog)
      watchDog--; // если сторожевая собака еще не разозлилась то дразним ее
    else          // если собака гавкает то принимаем меры
    {
      telomerases(RPI, currID, DEL); // наказываем не ответившую плату снижением
                                     // ее рейтинга на единицу
      nextID();                      // ищем следующую живую плату
      raptorOut(currID); // отправляем запрос следующей найденной плате
      telomerases(RAPTOR, currID, DEL); // отнимаем одну жизнь у платы которой
                                        // только что отправили запрос
      watchDog = WATCH_DOG; // подкармливаем и успокаиваем сторожевую собаку
    }
    p2 = t2;
  }
}

inline void telomerases(uint8_t board, uint8_t IDs, uint8_t operation) {
  if (board) // RPI
  {
    if (operation) // DEL
    {
      if (telomereRPI[IDs])
        telomereRPI[IDs]--; // снижаем рейтинг не ответившей плате на единицу
      else
        telomereRPiMap &=
            ~((uint32_t)1 << IDs); // если плата оборзела и вообще не отвечает
                                   // то запрещаем ей общаться с RPI
    } else                         // ADD
    {
      telomereRPI[IDs] = 5; // восстанавливаем рейтинг платы
      telomereRPiMap |=
          ((uint32_t)1 << IDs); // даем разрешение плате общаться с RPI
    }
  } else // RAPTOR
  {
    if (operation) // DEL
    {
      if (telomereRaptor[IDs])
        telomereRaptor[IDs]--; // если плата еще нужна серверу то отнимаем у нее
                               // одну жизнь
      else
        telomereRaptorMap &=
            ~(
                (uint32_t)1
                << IDs); // если сервер бросил плату и она стала совсем ненужна
                         // и жизни у нее кончились то хороним ее совсем
    } else               // ADD
    {
      telomereRaptor[IDs] = 20; // восстанавливаем все жизни плате
      telomereRaptorMap |= ((uint32_t)1 << IDs); // объявляем плату живой
    }
  }
}

inline void nextID() {
  do
    currID = ++currID & ID_MASK;
  while (!(telomereRaptorMap & ((uint32_t)1 << currID)));
}

/**
 * @brief Двигаем хвост змеи на 1
 */
inline void tailRpiShift() { tailrpi = ((tailrpi + 1) & ROUND_MASK); }

/**
 * @brief Считаем контрольную сумму
 *
 */
inline uint8_t getControlSumm() {
  stringSumm = 0;
  for (int i = 0; i < 40; i++)
    if (i != 7)
      stringSumm += roundBufferRPI[(tailrpi + i) & ROUND_MASK];
  return stringSumm;
}
void priemrpi() {

  // Данных нет - выходим
  if (!Serial2.available())
    return;

  uint8_t headIndex = headrpi();
  roundBufferRPI[headIndex] = Serial2.read();

  if (!((roundBufferRPI[tailrpi] == 0x51) &&
        (roundBufferRPI[(tailrpi + 1) & ROUND_MASK] == 0x42) &&
        (roundBufferRPI[(tailrpi + 2) & ROUND_MASK] == 0x52))) {
    tailRpiShift();
    return;
  }

  // Считаем сумму и если сумма не совпадает, выходим
  if (!(getControlSumm() == roundBufferRPI[(tailrpi + 7) & ROUND_MASK])) {
    tailRpiShift();
    return;
  }

  // Считываем id устройства
  uint8_t id = roundBufferRPI[(tailrpi + 5) & ROUND_MASK];

  // Восстанавливаем все жизни плате к которой обратился RPI
  if (id != DEVICE_ID)
    telomerases(RAPTOR, id, ADD);

  // Начинаем доить из RPI полезные данные для платы
  memcpy(&dataRPI[id], &roundBufferRPI[(tailrpi + 8) & ROUND_MASK], 32);

  if (telomereRPiMap & ((uint32_t)1 << id)) {
    // Если плата хорошо себя вела то отправляем ответ
    // малине на запрошенные данные об этой плате
    uint8_t counter = roundBufferRPI[(tailrpi + 4) & ROUND_MASK];
    rpiOut(id, counter);
  }

  tailRpiShift();
}

/**
 * @brief Получение нагрузки из шины
 * Почему priem? Так исторически сложилось
 */
void priem() {
  // Данных нет - выходим
  if (!Serial1.available())
    return;
  // Пишем в голову принятый байт
  uint8_t headIndex = head();
  roundBuffer[headIndex] = Serial1.read();

  // Eсли голова равна проценту и хвост равен звездочке то проверяем дальше
  if (!((roundBuffer[headIndex] == 0x25) & (roundBuffer[tail] == 0x2A))) {
    tailShift();
    return;
  }

  // Если совпадает айди запроса с нашим айди
  const uint8_t busID = roundBuffer[(tail + 2) & ROUND_MASK];
  if (busID != currID) {
    tailShift();
    return;
  }

  crcBuff[0] = busID;
  const uint8_t tailSum = tail + 3;

  // Высасываем полезные данные в отдельный массив
  for (int z = 0; z < 32; z++) {
    uint8_t roundMaskIndex = (tailSum + z) & ROUND_MASK;
    crcBuff[z + 1] = roundBuffer[roundMaskIndex];
    dataRaptor[currID][z] = roundBuffer[roundMaskIndex];
  }

  // Если контрольная сумма соответствет то действуем дальше
  if (roundBuffer[(tail + 1) & ROUND_MASK] != crc8(crcBuff, 33)) {
    tailShift();
    return;
  }

  telomerases(RPI, currID, ADD); // наша плата умничка, ответила нам,
  // восстанавливаем её рейтинг
  watchDog = WATCH_DOG; // подкармливаем и успокаиваем сторожевую собаку

  // если есть хоть одна живая плата то идем дальше
  if (telomereRaptorMap) {
    // ищем следующую живую плату
    nextID();
    // отправляем запрос следующей найденной плате
    raptorOut(currID);
    // отнимаем одну жизнь у платы которой только что отправили запрос
    telomerases(RAPTOR, currID, DEL);
  }

  // Двигаем хвост змеи
  tailShift();
}
/**
 * @brief Двигаем хвост змеи на 1
 */
inline void tailShift() { tail = ((tail + 1) & ROUND_MASK); }

inline uint8_t headrpi() {
  return (tailrpi + SNAKE_LENGTHRPI) &
         ROUND_MASK; // прибавляем к хвосту условную длину змейки и накладывем
                     // маску от переполнения. Получаем индекс головы
}

inline uint8_t head() {
  // прибавляем к хвосту условную длину змейки и накладывем
  // маску от переполнения. Получаем индекс головы
  return (tail + SNAKE_LENGTH) & ROUND_MASK;
}

inline void rpiOut(uint8_t id, uint8_t counter) {
  stringSumm = 0;
  outRPI[5] = id;
  outRPI[4] = counter;
  for (int i = 0; i < 40; i++) {
    if (i < 32)
      outRPI[i + 8] = dataRaptor[id][i];
    if (i != 7)
      stringSumm += outRPI[i]; // V
  }
  outRPI[7] = (uint8_t)stringSumm;
  Serial2.write(outRPI, 40);
}

inline void raptorOut(uint8_t id) {
  outRaptors[2] = id;
  crcBuff[0] = id; //  2 байт ID устройства

  memcpy(&outRaptors[3], &dataRPI[id], 32);
  memcpy(&crcBuff[1], &dataRPI[id], 32);

  outRaptors[1] = crc8(crcBuff, 33); //  0 - 33 байт данные //  1 байт равен
                                     //  контрольной сумме
  Serial1.write(outRaptors, LENGHT_ARRAY_OUT);
}

void startSetupADC() {
  ADCSRA &= ~(1 << ADPS0); //  Частота дискретизации
  ADCSRA &= ~(1 << ADPS1); //  Частота дискретизации
  ADCSRA |= (1 << ADPS2);  //  Частота дискретизации

  ADMUX |= (1 << REFS0);  //  Vref напряжение питания МК
  ADMUX &= ~(1 << ADLAR); //  Правостороннее выравнивание (стандартно)
  ADCSRA |= (1 << ADEN);  // Включаем АЦП
}

inline void readADC() {
  if (!(ADCSRA & (1 << ADSC))) {
    adc = adc & ADC_MASK;
    if (adc == 0) {
      adc = 10;
      for (uint8_t i = 0; i < 6; i++) {
        setBits(16 + 10 * i, 10, adcBuffer[i]);
      }
    }
    adcBuffer[adc - 10] = ADC; // Записываем значение в массив
    ADCSRB = (ADCSRB & 0xF7) | (adc & 0x08);
    ADMUX = (ADMUX & 0xE0) | (adc & 0x07);
    ADCSRA |= (1 << ADSC); //  Запускаем АЦП
    adc++;
  }
}

inline void pwmOuts(uint8_t pwm, uint8_t out) {
  switch (out) {
  case 1:
    OCR5B = pwm; // 1
    break;
  case 2:
    OCR5A = pwm; // 2
    break;
  case 3:
    OCR5C = pwm; // 3
    break;
  case 4:
    OCR2B = pwm; // 4
    OCR2B ? TCCR2A |= (1 << COM2B1) : TCCR2A &= ~(1 << COM2B1);
    break;
  }
}
