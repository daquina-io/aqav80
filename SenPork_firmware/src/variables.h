const char *mqtt_broker = "iot.unloquer.org";// broker address
const char *topic = "datos/sensor"; // define topic 
const char *temperatura = "datos/temperatura"; // define topic 
const char *humedad = "datos/humedad"; // define topic 
const char *aire = "datos/aire"; // define topic 
const char *sonido = "datos/sonido"; // define topic 
const char *co2 = "datos/co2"; // define topic 
const char *estado_on = "datos/on"; // define topic 
const char *estado_off = "datos/off"; // define topic 

const char *mqtt_username = ""; // username for authentication
const char *mqtt_password = "";// password for authentication
const int mqtt_port = 1883;// port of MQTT over TCP

#define SENSOR_ID "v80_aprendiedo"

#define FIXED_LAT "6.286984"
#define FIXED_LON "-75.508214"

//WiFi
//const char *wifi_ssid = "Daniel";
//const char *wifi_password = "24752475";