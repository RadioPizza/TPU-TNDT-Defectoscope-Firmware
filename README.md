![Status](https://img.shields.io/badge/Status-Active%20development-orange)
![License](https://img.shields.io/badge/License-GPL--3.0-green)
![Platform](https://img.shields.io/badge/Platform-ESP32--S3-blue)
![Arduino](https://img.shields.io/badge/Framework-Arduino%20Core-red)

# TPU-TNDT-Defectoscope-Firmware

Прошивка для микроконтроллера ESP32-S3 в составе теплового дефектоскопа. Контроллер выполняет функции управления питанием нагревательных элементов (ламп), эмуляции USB-устройств ввода (USB-HID) и обмена данными с ПК по COM-порту (через USB-UART WCH CH343).

## Функционал

- Хранение информации об аппаратной части дефектоскопа: ревизия аппаратной части, версия прошивки, калибровочные параметры джойстика
- Дискретное независимое управление 2 парами нагревательных ламп
- ШИМ-управление LED-подсветкой рабочей области
- ШИМ-управление индикатором статус (встроен в кнопку на правой рукояти)
- Опрос джойстика с кнопкой на левой рукояти
- Опрос кнопки на правой рукояти
- USB-HID эмуляция мыши
- USB-HID эмуляция клавиатуры
- Приём и исполнение команд управления через виртуальный COM-порт
- Отправка телеметрии и подтверждений в десктопное приложение
- Отслеживание текущего состояния питания устройства (устройство включено/выключено)

## Аппаратная часть

- Отладочная плата ESP32-S3-DevKitC-1. Модуль WROOM1 ESP32-S3-N16R8

  <img width="800" height="auto" alt="image" src="https://mischianti.org/wp-content/uploads/2023/08/vcc-gnd-studio-yd-esp32-s3-devkitc-1-clone-pinout-mischianti-high-resolution-1.png" />

  - Xtensa® dual-core 32-bit LX7 (до 240 МГц)
  
- 2 твердотельных реле для управления лампами JT-SSR-25A 
  - Пины подключения: 20 - правое, 19 - левое
Лампы по 500 Вт с патроном GY-9,5

- Джойстик K-Silver JH16 с датчиками Холла

  <img width="360" height="400" alt="image" src="https://github.com/user-attachments/assets/6728a280-a5ec-4a82-81b3-e4b229a821de" />

  - Пины подключения: 1 - осьX (АЦП1), 2 - осьY (АЦП1), 42 - кнопка (вход с подтяжкой)

- Кнопка с подсветкой для индикации на 3,3 В. Внутри аппаратного антидребезга вероятно нет. Снаружи тоже не предусматривается. Лучше сделать цифровой антидребезг 50-75 мс.
  - Пины подключения: 4 - земля (цифровая), 5 - кнопка (вход с подтяжкой), 6 - светодиод (выход ШИМ)

- LED-подсветка рабочей области: два белых светодиодов мощность 200 мВт. Номинальный ток 20 мА при напряжении 3 В. Подключаются напрямую к плате.
Пины подключения:
  Светодиод №1: 36 - земля (цифровая), 37 - сигнальный.
  Светодиод №2: 8 - земля (цифровая), 3 - сигнальный.
  В программе пины должны устанавливаться в уровень тока GPIO_CAP_3

Для упрощения отладки добавлен режим цифрового отключения ламп. Реализуется в виде переключателя или установкой перемычки на пины 9 (GND), 10 (вход с подтяжкой). Если пины замкнуты, то устройство работает в обычном режиме. При размыкании - команды на включение ламп игнорируются контроллером.

## Связь с приложением

Контроллер работает в паре с десктопным приложением **[TPU-TNDT-DualCam-App](https://github.com/RadioPizza/TPU-TNDT-DualCam-App)**

### Канал 1: управление и телеметрия (USB ↔ UART)

| Направление | Назначение |
|-------------|------------|
| **ПК → Контроллер** | Поиск контроллера, телеуправление, запрос состояния |
| **Контроллер → ПК** | Подтверждение принятия команд, телеметрия |

 **Интерфейс**: виртуальный COM-порт (USB-UART на базе WCH CH343)
- **Скорость**: 115200 бод
- **Протокол**: текстовый, строковый, с разделителем `\n`, без шифрования, человекочитаемый
- **Надёжность**: в разработке — пакеты с контрольной суммой и подтверждением доставки (см. [Issue #53](https://github.com/RadioPizza/TPU-TNDT-DualCam-App/issues/53))

### Канал 2: управление интерфейсом (USB-HID)

Контроллер эмулирует стандартные USB-устройства ввода:

- 🖱️ **Мышь**: джойстик на левой рукоятке управляет курсором; кнопка джойстика — ЛКМ, удержание кнопки джойстика — ПКМ
- ⌨️ **Клавиатура** (в разработке): физическая кнопка «Старт» на правой рукоятке эмулирует нажатие сочения клавиш для запуска/остановки процесса контроля

> 💡 **Преимущество**: оператор может полностью управлять приложением, не отпуская рукоятки дефектоскопа

## Схема электрическая структурная (Э1)

![Структурная схема дефектоскопа](docs/diagrams/ФЮРА.412239.001%20Э1.svg)


# Протокол связи ESP32 ↔ Python (планшет)

## Общая идея

Связь по последовательному порту (UART / USB). Поверх потока байт используется
**COBS-фреймирование** — оно разбивает поток на чёткие пакеты без случайных
разрывов. Библиотека **PacketSerial** на ESP32 и пакет **cobs** на Python делают
это автоматически — ты работаешь уже с готовыми байтовыми массивами.

```
Физический уровень:  UART / USB Serial  (115200 baud)
Фреймирование:       COBS  (PacketSerial / cobs)
Протокол:            свой бинарный (описан ниже)
```

---

## Структура пакета

```
[ CMD : 1 байт ][ SEQ : 1 байт ][ DATA : N байт ][ CRC16 : 2 байта ]
```

| Поле  | Размер   | Описание                                                  |
|-------|----------|-----------------------------------------------------------|
| CMD   | 1 байт   | Тип команды/события (см. таблицу ниже)                   |
| SEQ   | 1 байт   | Порядковый номер пакета. Счётчик 0–255, потом по кругу   |
| DATA  | 0–N байт | Полезная нагрузка. Длина зависит от типа команды          |
| CRC16 | 2 байта  | Контрольная сумма байт CMD+SEQ+DATA (big-endian)          |

Пакет без корректного CRC молча отбрасывается получателем.

---

## Таблица команд

### ESP32 → Планшет (события и ответы)

| CMD  | Название     | DATA                             | Описание                            |
|------|--------------|----------------------------------|-------------------------------------|
| 0x01 | BTN_EVENT    | [btn_id: 1 байт, state: 1 байт] | Нажатие/отпускание кнопки           |
| 0x02 | STATUS_RESP  | [power, sys1, sys2, light]       | Ответ на запрос статуса             |
| 0xFF | ACK          | [seq: 1 байт]                    | Подтверждение получения пакета      |
(здесь power - состояние питания устройства, sys1, sys2 - сборки ламп слева и справа, light - подсветка)

**BTN_EVENT, поле state:** `0x01` = нажата, `0x00` = отпущена  
**STATUS_RESP, каждое поле:** `0x01` = включено/есть, `0x00` = выключено/нет

### Планшет → ESP32 (команды)

| CMD  | Название      | DATA                          | Описание                          |
|------|---------------|-------------------------------|-----------------------------------|
| 0x10 | CMD_START     | [system: 1 байт]              | Запустить систему (1 или 2)       |
| 0x11 | CMD_STOP      | [system: 1 байт]              | Остановить систему (1 или 2)      |
| 0x12 | CMD_LIGHT     | [state: 1 байт]               | Включить/выключить подсветку      |
| 0x20 | STATUS_REQ    | —                             | Запросить текущий статус          |
| 0xFF | ACK           | [seq: 1 байт]                 | Подтверждение получения пакета    |

---

## Правила обмена

**Каждый пакет требует ACK.** Отправитель ждёт ACK не дольше 200 мс,
затем повторяет пакет. Максимум 3 попытки, после — ошибка соединения.

**Защита от дублей по SEQ.** Получатель помнит последний обработанный SEQ.
Если пришёл пакет с тем же SEQ — отправляет ACK, но повторно не обрабатывает.

**Решения принимает только планшет.** ESP32 сообщает о событиях (кнопка нажата)
и выполняет команды, но никогда не запускает системы самостоятельно.

**Планшет обновляет интерфейс только после ACK** от ESP32 на свою команду —
не раньше, чтобы интерфейс не врал при потере пакета.

**Плановый опрос статуса** — планшет отправляет STATUS_REQ раз в секунду
как страховку от рассинхронизации.

**Внеплановое событие** — при критичном изменении (например, пропало питание)
ESP32 не ждёт опроса, а сразу шлёт STATUS_RESP или специальное BTN_EVENT.

---

## Примеры диалогов

### Нажатие кнопки → запуск системы 1

```
ESP32   → Планшет:  CMD=0x01  SEQ=5   DATA=[btn_id=1, state=0x01]   # кнопка нажата
Планшет → ESP32:    CMD=0xFF  SEQ=0   DATA=[seq=5]                   # ACK на событие
Планшет → ESP32:    CMD=0x10  SEQ=42  DATA=[system=1]                # запустить систему 1
ESP32   → Планшет:  CMD=0xFF  SEQ=0   DATA=[seq=42]                  # ACK на команду

ESP32   → Планшет:  CMD=0x01  SEQ=6   DATA=[btn_id=1, state=0x00]   # кнопка отпущена
Планшет → ESP32:    CMD=0xFF  SEQ=0   DATA=[seq=6]                   # ACK на событие
Планшет → ESP32:    CMD=0x11  SEQ=43  DATA=[system=1]                # остановить систему 1
ESP32   → Планшет:  CMD=0xFF  SEQ=0   DATA=[seq=43]                  # ACK на команду
```

### Включение подсветки с планшета

```
Планшет → ESP32:    CMD=0x12  SEQ=44  DATA=[state=0x01]              # включить подсветку
ESP32   → Планшет:  CMD=0xFF  SEQ=0   DATA=[seq=44]                  # ACK — выполнено
# Планшет обновляет UI только здесь
```

### Плановый опрос статуса

```
Планшет → ESP32:    CMD=0x20  SEQ=45  DATA=[]                        # запрос статуса
ESP32   → Планшет:  CMD=0xFF  SEQ=0   DATA=[seq=45]                  # ACK на запрос
ESP32   → Планшет:  CMD=0x02  SEQ=7   DATA=[power=1,sys1=1,sys2=0,light=1]
Планшет → ESP32:    CMD=0xFF  SEQ=0   DATA=[seq=7]                   # ACK на ответ
```

---

## Код — ESP32 (C++)

```cpp
#include <PacketSerial.h>

PacketSerial packetSerial;
uint8_t txSeq = 0;       // счётчик исходящих пакетов
uint8_t lastRxSeq = 255; // последний обработанный входящий SEQ

// --- CRC16 (CRC-CCITT) ---
uint16_t crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
    return crc;
}

// --- Отправка пакета ---
void sendPacket(uint8_t cmd, const uint8_t* data, size_t dataLen) {
    uint8_t buf[64];
    buf[0] = cmd;
    buf[1] = txSeq++;                          // порядковый номер
    memcpy(buf + 2, data, dataLen);
    uint16_t crc = crc16(buf, 2 + dataLen);
    buf[2 + dataLen]     = crc >> 8;           // CRC старший байт
    buf[2 + dataLen + 1] = crc & 0xFF;         // CRC младший байт
    packetSerial.send(buf, 4 + dataLen);        // PacketSerial сам добавит COBS
}

// --- Отправка ACK ---
void sendAck(uint8_t seq) {
    uint8_t data[] = { seq };
    sendPacket(0xFF, data, 1);
}

// --- Callback: пришёл пакет от планшета ---
void onPacketReceived(const uint8_t* buf, size_t size) {
    if (size < 4) return;                      // минимальный размер: CMD+SEQ+CRC

    // Проверка CRC
    uint16_t receivedCrc = (buf[size-2] << 8) | buf[size-1];
    if (crc16(buf, size - 2) != receivedCrc) return;  // битый пакет — игнорируем

    uint8_t cmd = buf[0];
    uint8_t seq = buf[1];

    // Защита от дублей
    if (cmd != 0xFF && seq == lastRxSeq) {
        sendAck(seq);   // ACK отправляем, но не обрабатываем повторно
        return;
    }
    lastRxSeq = seq;

    // Подтверждаем получение (кроме самих ACK)
    if (cmd != 0xFF) sendAck(seq);

    // Обработка команд
    const uint8_t* payload = buf + 2;         // DATA начинается с 3-го байта
    switch (cmd) {
        case 0x10:                             // CMD_START
            startSystem(payload[0]);           // payload[0] = номер системы
            break;
        case 0x11:                             // CMD_STOP
            stopSystem(payload[0]);
            break;
        case 0x12:                             // CMD_LIGHT
            setLight(payload[0]);              // payload[0] = 0x01/0x00
            break;
        case 0x20:                             // STATUS_REQ — ответить статусом
            sendStatus();
            break;
    }
}

// --- Отправить статус ---
void sendStatus() {
    uint8_t data[] = {
        getPower(),   // 0x01 или 0x00
        getSys1(),
        getSys2(),
        getLight()
    };
    sendPacket(0x02, data, sizeof(data));
}

// --- Отправить событие кнопки ---
void sendButtonEvent(uint8_t btnId, bool pressed) {
    uint8_t data[] = { btnId, pressed ? 0x01 : 0x00 };
    sendPacket(0x01, data, sizeof(data));
    // После отправки нужно ждать ACK (логику ожидания добавить отдельно)
}

void setup() {
    packetSerial.begin(115200);
    packetSerial.setPacketHandler(&onPacketReceived);
}

void loop() {
    packetSerial.update();   // читает поток и вызывает onPacketReceived при готовом пакете
    checkButtons();          // опрос кнопок → sendButtonEvent при изменении
}
```

---

## Код — Python (планшет)

```python
import serial
import struct
import threading
from cobs import cobs

CMD_BTN_EVENT  = 0x01
CMD_STATUS_RESP = 0x02
CMD_START      = 0x10
CMD_STOP       = 0x11
CMD_LIGHT      = 0x12
CMD_STATUS_REQ = 0x20
CMD_ACK        = 0xFF

def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = (crc << 1) ^ 0x1021 if crc & 0x8000 else crc << 1
    return crc & 0xFFFF

class DeviceProtocol:
    def __init__(self, port: str, baudrate: int = 115200):
        self.ser = serial.Serial(port, baudrate, timeout=0.1)
        self.tx_seq = 0
        self.last_rx_seq = None       # защита от дублей
        self.pending_acks = {}        # seq → callback, ожидающие подтверждения
        self._buf = bytearray()
        self._lock = threading.Lock()

    # --- Отправка пакета ---
    def send(self, cmd: int, data: bytes = b'', on_ack=None):
        with self._lock:
            seq = self.tx_seq & 0xFF
            self.tx_seq += 1

        payload = bytes([cmd, seq]) + data
        crc = crc16(payload)
        payload += struct.pack('>H', crc)           # CRC big-endian

        encoded = cobs.encode(payload) + b'\x00'    # \x00 — разделитель пакетов
        self.ser.write(encoded)

        if on_ack:
            self.pending_acks[seq] = on_ack         # запомнить callback для ACK

    def send_ack(self, seq: int):
        self.send(CMD_ACK, bytes([seq]))

    # --- Петля чтения (запускать в отдельном потоке) ---
    def read_loop(self):
        while True:
            byte = self.ser.read(1)
            if not byte:
                continue
            if byte == b'\x00':                     # конец COBS-пакета
                if self._buf:
                    try:
                        decoded = cobs.decode(bytes(self._buf))
                        self._handle_packet(decoded)
                    except Exception:
                        pass                        # битый пакет — игнорируем
                    self._buf.clear()
            else:
                self._buf += byte

    def _handle_packet(self, data: bytes):
        if len(data) < 4:
            return

        cmd, seq = data[0], data[1]
        payload  = data[2:-2]
        received_crc = struct.unpack('>H', data[-2:])[0]

        if crc16(data[:-2]) != received_crc:
            return                                  # битый CRC — отбрасываем

        # Защита от дублей
        if cmd != CMD_ACK and seq == self.last_rx_seq:
            self.send_ack(seq)
            return
        self.last_rx_seq = seq

        # Подтверждение (кроме самих ACK)
        if cmd != CMD_ACK:
            self.send_ack(seq)

        # Обработка входящих пакетов
        if cmd == CMD_ACK:
            ack_seq = payload[0]
            if ack_seq in self.pending_acks:
                callback = self.pending_acks.pop(ack_seq)
                callback()                          # вызвать callback подтверждения

        elif cmd == CMD_BTN_EVENT:
            btn_id = payload[0]
            pressed = payload[1] == 0x01
            self.on_button(btn_id, pressed)         # переопределить в наследнике

        elif cmd == CMD_STATUS_RESP:
            power, sys1, sys2, light = payload[0], payload[1], payload[2], payload[3]
            self.on_status(power, sys1, sys2, light)

    # --- Публичные методы отправки команд ---
    def start_system(self, system: int, on_ack=None):
        self.send(CMD_START, bytes([system]), on_ack)

    def stop_system(self, system: int, on_ack=None):
        self.send(CMD_STOP, bytes([system]), on_ack)

    def set_light(self, state: bool, on_ack=None):
        self.send(CMD_LIGHT, bytes([0x01 if state else 0x00]), on_ack)

    def request_status(self):
        self.send(CMD_STATUS_REQ)

    # --- Переопределить в наследнике ---
    def on_button(self, btn_id: int, pressed: bool):
        pass

    def on_status(self, power: int, sys1: int, sys2: int, light: int):
        pass


# --- Пример использования ---
class MyDevice(DeviceProtocol):
    def on_button(self, btn_id, pressed):
        print(f"Кнопка {btn_id}: {'нажата' if pressed else 'отпущена'}")
        if btn_id == 1 and pressed:
            # Планшет принимает решение и отправляет команду
            self.start_system(1, on_ack=lambda: print("Система 1 запущена"))
        elif btn_id == 1 and not pressed:
            self.stop_system(1)

    def on_status(self, power, sys1, sys2, light):
        print(f"Питание={power} Сис1={sys1} Сис2={sys2} Свет={light}")


if __name__ == "__main__":
    device = MyDevice('/dev/ttyUSB0')

    # Читаем порт в фоновом потоке
    t = threading.Thread(target=device.read_loop, daemon=True)
    t.start()

    # Запрашиваем статус раз в секунду
    import time
    while True:
        device.request_status()
        time.sleep(1)
```

---

## Зависимости

**ESP32:**
```
PacketSerial  (через Arduino Library Manager или platformio)
```

**Python:**
```
pip install pyserial cobs
```

---

## Быстрая памятка

| Ситуация                        | Кто действует первым |
|---------------------------------|----------------------|
| Кнопка нажата                   | ESP32 → BTN_EVENT    |
| Запуск/остановка системы        | Планшет → CMD_START / CMD_STOP |
| Нужен текущий статус            | Планшет → STATUS_REQ |
| Критичное изменение на железе   | ESP32 → STATUS_RESP (внеплановый) |
| Любой принятый пакет            | Получатель → ACK     |
