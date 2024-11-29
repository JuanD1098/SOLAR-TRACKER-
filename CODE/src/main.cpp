#include <Arduino.h>
#include <Servo.h>
#include <Adafruit_GFX.h>       // Librería para gráficos OLED
#include <Adafruit_SSD1306.h>   // Librería específica para pantalla OLED

// Configuración de pantalla OLED
#define SCREEN_WIDTH 128   // Ancho de la pantalla OLED en píxeles
#define SCREEN_HEIGHT 64   // Alto de la pantalla OLED en píxeles
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // Inicializa OLED con I2C

// Inicialización de servos
Servo myservo1, myservo2; 

// Pines de los sensores LDR conectados al ESP32
int LDR1 = 32, LDR2 = 33, LDR3 = 25, LDR4 = 26; 

// Variables para almacenar lecturas de LDR
int rRDL1 = 0, rRDL2 = 0, rRDL3 = 0, rRDL4 = 0; 

// Variables para almacenar los valores máximos de las lecturas
int max1 = 0, max2 = 0, max3 = 0;

// Posiciones iniciales de los servos
int ser1 = 80, ser2 = 0; 

// Parámetros para medición de voltaje y corriente
const int Vmax = 25000;         // Voltaje máximo en milivoltios
int lecDig;                     // Variable para lectura del ADC
float vol;                      // Variable para almacenar el voltaje leído

const int CURRENT_SENSOR = 27;  // Pin conectado al sensor de corriente ACS712
const float SENSIBILITY = 0.185; // Sensibilidad del sensor (para modelo 5A)
const int CURRENT_SAMPLES = 10; // Número de muestras para promedio

// Variables para control de tiempo
unsigned long lastTime = 0;
unsigned long threshold = 2000; // Intervalo de 2 segundos

// Función para medir corriente usando el sensor ACS712
float medirCorriente(int current_sensor, float sensibility, int samples) {
  float sensor_read = 0;

  // Lee múltiples muestras del sensor y calcula el promedio
  for (int i = 0; i < samples; i++) {
    sensor_read += analogRead(current_sensor);
  }
  sensor_read = sensor_read / samples;

  // Convierte la lectura promedio en voltaje (considerando 12 bits y 3.3V de referencia)
  sensor_read = sensor_read * (3.3 / 4095.0);

  // Calcula la corriente basándose en el voltaje y la sensibilidad del sensor
  float current_measured = (sensor_read - 2.5) / sensibility;
  return current_measured;
}

// Función para mostrar información en la pantalla OLED (voltaje, corriente y ángulos)
void mostrarDatosEnOLED(float vol, float current_measured) {
  display.clearDisplay();  // Limpia la pantalla antes de escribir
  display.setTextSize(1);  // Tamaño de texto pequeño
  display.setTextColor(SSD1306_WHITE);  // Texto en blanco

  // Mostrar el voltaje
  display.setCursor(0, 0);
  display.print("VOLTAGE: ");
  display.print(vol, 2);  // Muestra el voltaje con 2 decimales
  display.print("V");

  // Mostrar la corriente
  display.setCursor(0, 10);
  display.print("CURRENT: ");
  display.print(current_measured, 2);  // Muestra la corriente con 2 decimales
  display.print("A");

  // Mostrar el ángulo del servo 1
  display.setCursor(0, 20);
  display.print("Servo 1: ");
  display.print(ser1);
  display.print(" deg");

  // Mostrar el ángulo del servo 2
  display.setCursor(0, 30);
  display.print("Servo 2: ");
  display.print(ser2);
  display.print(" deg");

  display.display();  // Actualiza la pantalla OLED
}

void setup() {
  // Inicialización de servos
  myservo1.attach(12); // Conecta el servo 1 al pin 12
  myservo2.attach(14); // Conecta el servo 2 al pin 14

  // Inicializa la pantalla OLED
  if (!display.begin(0x3C)) { 
    Serial.println(F("OLED no encontrado!")); 
    while (true); // Detiene el programa si no se detecta la pantalla
  }

  display.clearDisplay(); // Limpia cualquier contenido inicial
  display.display(); // Actualiza la pantalla

  // Posiciones iniciales de los servos
  myservo1.write(ser1);
  myservo2.write(ser2);
}

void loop() {
  // Leer el voltaje del ADC
  lecDig = analogRead(13);  // Lectura del ADC 13
  vol = map(lecDig, 0, 4095, 0, Vmax) / 1000.0; // Convertir a voltaje en voltios

  // Medir corriente cada cierto tiempo
  if ((millis() - lastTime) > threshold) {
    lastTime = millis();  // Actualiza el tiempo

    float current_measured = medirCorriente(CURRENT_SENSOR, SENSIBILITY, CURRENT_SAMPLES);  // Medir corriente
    
    // Mostrar voltaje y corriente en OLED
    mostrarDatosEnOLED(vol, current_measured);
  }

  // Leer los valores de los sensores LDR
  rRDL1 = analogRead(LDR1) / 100;
  rRDL2 = analogRead(LDR2) / 100;
  rRDL3 = analogRead(LDR3) / 100;
  rRDL4 = analogRead(LDR4) / 100;

  // Comparar valores de sensores para ajustar servos
  max1 = max(rRDL1, rRDL2);
  max2 = max(rRDL3, rRDL4);
  max3 = max(max1, max2);

  if (rRDL1 < max3 && rRDL2 < max3) {
    if (ser1 < 140) ser1 += 1;  // Incrementa el ángulo del servo 1 si está por debajo de su límite
    myservo1.write(ser1);       // Actualiza la posición del servo 1
  }

  if (rRDL3 < max3 && rRDL4 < max3) {
    if (ser1 > 0) ser1 -= 1;    // Disminuye el ángulo del servo 1 si está por encima de su límite
    myservo1.write(ser1);
  }

  if (rRDL2 < max3 && rRDL3 < max3) {
    if (ser2 < 180) ser2 += 1;  // Incrementa el ángulo del servo 2 si está por debajo de su límite
    myservo2.write(ser2);
  }

  if (rRDL1 < max3 && rRDL4 < max3) {
    if (ser2 > 0) ser2 -= 1;    // Disminuye el ángulo del servo 2 si está por encima de su límite
    myservo2.write(ser2);
  }

  delay(20);  // Pausa para evitar cambios bruscos
}
