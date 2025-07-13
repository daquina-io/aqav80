## Agentes de la Calidad del Aire (AQAV80)
Cuando se desea profundizar en el problema de la calidad del aire en Medellín nos encontramos con varios obstáculos debido a la complejidad del problema y la limitada capacidad de los ciudadanos para recolectar y examinar las mediciones.
El Kit AQA se esfuerza por acercar al ciudadano a la posibilidad de medir y analizar el aire que respira. De hecho, en las actividades propuestas se aprende a ensamblar un dispositivo para la medición del materialparticulado, y se considera unas herramientas para el estudio de las mediciones.
Es en los detalles del ensamblaje y el uso de las herramientas de análisis donde se puede pensar con las manos y entrar en relación directa con el problema de la calidad del aire.

## Enlaces
  * [Ejemplo de la trama de datos](https://raw.githubusercontent.com/daquina-io/VizCalidadAire/master/data/points.csv) https://raw.githubusercontent.com/daquina-io/VizCalidadAire/master/data/points.csv
  * [Foro](https://comunidad.unloquer.org/) comunidad unloquer  https://comunidad.unloquer.org/
  * [wiki](http://wiki.unloquer.org/personas/brolin/proyectos/agentes_calidad_aire) wiki unloquer http://wiki.unloquer.org/personas/brolin/proyectos/agentes_calidad_aire
  * [mapa de mediciones](http://daquina.io/aqaviz/) daquia.io http://daquina.io/aqaviz/
  * [Licensed under the TAPR Open Hardware License](www.tapr.org/OHL): www.tapr.org/OHL


## ¿De donde vienen los datos? -> de estos sensores: 

 * dht11 --> temperatura y humedad 
 * plantower --> material particulado
 * max9814 --> ruido

## ¿Hacia donde salen los datos?

  * [mapa de mediciones](http://daquina.io/aqaviz/) daquia.io http://daquina.io/aqaviz/
  * [los dash board](http://aqa.unloquer.org:8888/sources/2/chronograf/data-explorer) chronograf http://aqa.unloquer.org:8888/sources/2/chronograf/data-explorer
  en la parte de  v80.autogen

## ¿Como comprender el codigo de colores de los LEDs?
el color gradiente de los leds funciona mientras mas intenso el color mas contaminacion  y menos inteso el color  menos contaminacion

el color gradiente de los leds funciona mientras mas intenso el color mas contaminacion y menos inteso el color menos contaminacion 

color > 13 resultado genrlamente verde 

13 < color < 35 resultado  generamente amarillo

35 < color < 55 resultado  generalmente naranja

55 < color < 75 resultado  generalmente rojo

75 < color < 255 resultado  generalmente morado

color < 255 marron

## ¿Como lo instalo? 

Descarga el repositorio

          git clone -b AQAV80_modular https://github.com/jero98772/aqav80.git


Usando Visual Studio Code, aka Code en sistemas GNU/Linux. Es tan facil como instalar Platformio y asegurarse que en el archivo platformio.ini se nombre el dispositivo que queremos flashear:  upload_port = /dev/ttyUSB0  

Lea aqui: https://comunidad.unloquer.org/t/como-programar-el-aqa/33/7

En el archivo main.cpp se deben cambiar los valores para ubicacion y datos de la red que se utilizara para enviar los datos capturados, entre los cuales estan, por ejemplo estos:

#define FIXED_LAT "6.167222"  
#define FIXED_LON "-75.426667"  
#define SENSOR_ID "Rionegro"  

y, mas abajo en el mismo archivo:


#define WIFI_SSID "ESSID/Nombre de la red"  
#define WIFI_PASS "PASSWORD/Clave de la red"  

Para otras maneras de instalarlo lea aqui: 
[enlace a como instalarlo en comunidad unloquer](https://comunidad.unloquer.org/t/cargar-el-firmware-desde-linea-de-comando/118) 

#### un breve resumen paso a paso

1. configurar el sensor en el archivo scr/main.cpp , recuerde guardar cambios
2. $ pio run  ,para compilar  el codigo con las nuevas configuraciones . recurde instalar [platformio](https://pypi.org/project/platformio/)
3. $ esptool.py --port /dev/ttyUSB0  write_flash 0x00000 .pio/build/d1_mini/firmware.bin ,el puerto no siempre es /dev/ttyUSB0 , recuerde instalar [esptool](https://pypi.org/project/esptool/)

mas informacion [aqui](https://comunidad.unloquer.org)

## como puedo especificar los modulos y modos que voy a usar?
puedes especificar que vas a usar dentro de estas macros

	#define SENSOR_ID "aprendiedo"
	#define INTERNET
	#define DHT_SENSOR
	#define MIC
	#define APP
	#define GPS
	#define MAP
	#define LED
	#define LED_CODE
	#define DEBUGGING

cuando espcificas el puede ser mas eficiente que otros por que se hace un binario decuerdo a lo que nesesitas 

es diferente a no usar algunas funciones que estan consumiedo recursos en la memoria

espesificando con macros no se compilan esas funciones entonces no van haber problemas de funciones en desuso consumiedo recursos 

especificando el tipo de sesnor "aqav80" seria :
 
	#define SENSOR_ID "v80_aprendiedo"
	#define INTERNET
	#define DHT_SENSOR
	#define MIC
	//#define APP
	//#define GPS
	//#define MAP
	#define LED
	//#define LED_CODE
	//#define DEBUGGING


## Contacto 

 * irc --> #un/loquer en irc.freenode.net
 * twitter --> [twitter de unloquer](https://twitter.com/unloquer?lang=es) enlace a https://twitter.com/unloquer?lang=es

# SenPork Firmware

Environmental sensor firmware for the SenPork project.

## Features

- **Multiple CO2 Sensor Support**: Supports both MHZ19 and SenseAir S8 CO2 sensors with automatic detection
- **Multi-sensor monitoring**: Temperature, humidity, PM2.5, sound level, and CO2 measurements
- **WiFi connectivity** with automatic configuration
- **MQTT data publishing** for real-time monitoring
- **OTA firmware updates** with cryptographic signature verification
- **Task-based architecture** for efficient sensor sampling
- **Comprehensive logging** with configurable levels

## Supported Sensors

### CO2 Sensors
- **MHZ19**: Winsen MH-Z19 series CO2 sensors
- **SenseAir S8**: SenseAir S8 LP CO2 sensors with advanced autocalibration features

The firmware supports automatic detection of CO2 sensor types. If you need to specify a particular sensor type, you can configure it in the HAL initialization.

#### SenseAir S8 Autocalibration Features
- **Automatic Background Calibration (ABC)**: Continuously calibrates to outdoor air levels (400 ppm) over a 7-day period
- **Zero Point Calibration**: Manual calibration to fresh outdoor air (400 ppm)  
- **Span Calibration**: Manual calibration to known CO2 concentrations
- **Configurable calibration periods**: Set ABC period from hours to weeks

### Other Sensors
- **Temperature/Humidity**: SHT40 (I2C) or DHT22 (digital) with auto-detection
- **Particulate Matter**: PMS series sensors (PM2.5)
- **Sound Level**: Analog microphone sensors

## Hardware Setup

### CO2 Sensor Connections

#### MHZ19 Sensor
```
ESP32     MHZ19
-----     -----
3.3V  --> VCC
GND   --> GND
Pin16 --> TX
Pin17 --> RX
```

#### SenseAir S8 Sensor
```
ESP32     SenseAir S8
-----     -----------
5V    --> VCC
GND   --> GND
Pin16 --> TX
Pin17 --> RX
```

### Pin Configuration

The default pin configuration is defined in `src/hal/board/ttgo_t7_v14.h`:

```cpp
struct BoardPins {
    static const int DHT_PIN = 13;
    static const int I2C_SDA = 21;
    static const int I2C_SCL = 22;
    static const int ADC_SOUND = 35;
    static const int PM_TX = 34;
    static const int PM_RX = 14;
    static const int CO2_TX = 16;
    static const int CO2_RX = 17;
};
```

## Configuration

### CO2 Sensor Type Selection

The firmware automatically detects the CO2 sensor type by default. You can also specify a particular sensor type:

```cpp
// Auto-detection (default)
hal.initCO2Sensor(BoardPins::CO2_RX, BoardPins::CO2_TX, Serial2);

// Force MHZ19
hal.initCO2Sensor(BoardPins::CO2_RX, BoardPins::CO2_TX, Serial2, 
                  CO2Sensor::SensorType::MHZ19, false);

// Force SenseAir S8
hal.initCO2Sensor(BoardPins::CO2_RX, BoardPins::CO2_TX, Serial2, 
                  CO2Sensor::SensorType::SENSEAIR_S8, false);
```

### WiFi and MQTT Configuration

Configuration is handled through the `secrets.h` file (create from template):

```cpp
// WiFi Configuration
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// MQTT Configuration
#define MQTT_BROKER "your_mqtt_broker"
#define MQTT_PORT 1883
#define MQTT_USERNAME "your_username"
#define MQTT_PASSWORD "your_password"
```

## Build and Upload

1. Install PlatformIO
2. Clone the repository
3. Create `secrets.h` file with your configuration
4. Build and upload:

```bash
pio run --target upload
```

## Library Dependencies

The firmware uses the following libraries:

- `MH-Z19`: For MHZ19 CO2 sensors
- `S8_UART`: For SenseAir S8 CO2 sensors
- `ArduinoJson`: JSON handling for MQTT messages
- `PubSubClient`: MQTT communication
- `WiFiManager`: WiFi configuration management
- `TaskScheduler`: Task management
- `Sensirion I2C SHT4x`: SHT40 temperature/humidity sensor
- `DHT sensor library`: DHT22 temperature/humidity sensor
- `PMS Library`: Particulate matter sensor support

## API Reference

### CO2Sensor Class

```cpp
class CO2Sensor {
public:
    enum class SensorType {
        NONE,
        MHZ19,
        SENSEAIR_S8
    };
    
    // Initialize with auto-detection
    bool init(int rxPin, int txPin, HardwareSerial& serial, 
              SensorType type = SensorType::NONE, bool autoDetect = true);
    
    // Read sensor values
    bool read(int& co2, int8_t& temperature);
    bool readWithRetry(int& co2, int8_t& temperature, int maxRetries = 3, int retryDelayMs = 200);
    
    // Basic calibration
    void calibrate();
    
    // Advanced calibration (SenseAir S8 specific)
    bool enableAutoCalibration(bool enable = true);
    bool setAutoCalibrationPeriod(uint16_t hours = 168);
    bool performZeroPointCalibration();
    bool performSpanCalibration(uint16_t concentration);
    bool isAutoCalibrationEnabled();
    uint16_t getAutoCalibrationPeriod();
    
    // Status methods
    bool isInitialized() const;
    SensorType getType() const;
};
```

#### Calibration Examples

```cpp
// Enable automatic background calibration (recommended)
co2Sensor.enableAutoCalibration(true);
co2Sensor.setAutoCalibrationPeriod(168); // 7 days

// Manual zero point calibration (outdoor air)
// WARNING: Only use in fresh outdoor air (400 ppm)
co2Sensor.performZeroPointCalibration();

// Span calibration with reference gas
// WARNING: Only use with known reference concentration
co2Sensor.performSpanCalibration(2000); // 2000 ppm reference gas

// Check calibration status
bool abcEnabled = co2Sensor.isAutoCalibrationEnabled();
uint16_t period = co2Sensor.getAutoCalibrationPeriod();
```

## License

This project is licensed under the MIT License.
