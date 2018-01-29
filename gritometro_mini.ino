const String marcador = "lcd";

#include  <Wire.h>
#include  <LiquidCrystal_I2C.h>

#include "pitches.h" // Libreria de notas predefinidas

// Constructor de la librería de LCD 16x2
// Aqui se configuran los pines asignados a la pantalla del PCF8574
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

const int  buttonPin = 2;    // the pin that the pushbutton is attached to

/* Inicialización Neopixel */
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define PIN 6 // Pin digital tira led
#define NUMPIXELS 30 // Cantidad de leds

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
/* FIN - Inicialización Neopixel */

/* Inicialización sensor sonido */
const int inputPin = A0; // Pin analogico sensor audio

// Medir el valor del sonido
const int inputWindow = 100; // size of the window
unsigned int inputSample; // placeholder for a single measurement

// Medir el máximo umbral del sonido
unsigned int diference = 0;
unsigned int maxDiference = 0;
/* FIN - Inicialización sensor sonido */

int ultimoPorcentaje = 0; // Utilizado para saber si hay que apagar un pixel o no

long previousMillis = 0;        // will store last time LED was updated

// the follow variables is a long because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long interval = 1000;           // interval at which to blink (milliseconds)

boolean start = false;  // Bandera para saber si hay que empezar a registrar el "grito"
boolean end   = false;  // Bandera para saber si hay que finalizar el "grito"
int countdown = 3;      // Cuenta atras inicial
int timer     = 5;      // Duración del "grito"

// Variables will change:
int buttonState = 0;         // current state of the button
int lastButtonState = 0;     // previous state of the button
boolean startSequence = false;

void setup() {
  // initialize the button pin as a input:
  pinMode(buttonPin, INPUT);

  pixels.begin();           // This initializes the NeoPixel library.
  pinMode(inputPin, INPUT); // initializing the analog input
  Serial.begin(9600);       // initializing the serial communication

  if (marcador == "lcd") {
    // Indicar a la libreria que tenemos conectada una pantalla de 16x2
    lcd.begin(16, 2);
    // Mover el cursor a la primera posición de la pantalla (0, 0)
    lcd.home ();
  }
}

void loop() {
  // read the pushbutton input pin:
  buttonState = digitalRead(buttonPin);
  delay(50);

  Serial.println(buttonState);

  if (buttonState) {
    startSequence = true;
  }

  Serial.println(startSequence);

  // compare the buttonState to its previous state
  if (startSequence) {

    // check to see if it's time to blink the LED; that is, if the
    // difference between the current time and last time you blinked
    // the LED is bigger than the interval at which you want to
    // blink the LED.
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis > interval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;

      // Empieza la cuenta atras
      if (!start && !end) {
        if (countdown > 0) {
          // Cuenta atras
          tone(13, NOTE_C3, 1000 / 4);
          if (marcador == "lcd") {
            lcd.home ();
            lcd.print(countdown);
          }
          Serial.println(countdown);
          countdown--;
        } else {
          // GO
          tone(13, NOTE_C4, 1000 / 2);
          if (marcador == "lcd") {
            // Mover el cursor a la primera posición de la pantalla (0, 0)
            lcd.home ();
            lcd.print("Go");
          }
          Serial.println("Go");
          start = true;
        }
      }

      // Capturando sonido
      if (start && !end) {
        if (timer > 0) {
          // Capturando
          Serial.print("Tiempo restante: ");
          Serial.println(timer);
          if (marcador == "lcd") {
            lcd.setCursor ( 0, 1 );
            lcd.print("Tiempo: ");
            lcd.print(timer);
          }
          timer--;
        } else {
          // FIN
          end = true;
          Serial.println("Fin");
          Serial.println(maxDiference);
          int porcentaje = (maxDiference * 100) / 1024;
          encenderPixels(porcentaje);
          Serial.print("Porcentaje final leds: ");
          Serial.println(porcentaje);

          if (marcador == "lcd") {
            lcd.setCursor ( 0, 1 );
            lcd.print("   *** FIN ***  ");
          }

          endTone();
          startSequence = false;
          start = false;  // Bandera para saber si hay que empezar a registrar el "grito"
          end   = false;  // Bandera para saber si hay que finalizar el "grito"
          countdown = 3;      // Cuenta atras inicial
          timer     = 5;
        }
      }
    }

    if (start && !end) {
      // https://www.luisllamas.es/medir-sonido-con-arduino-y-microfono-amplificado-max9812/

      // two variables for minimum and maximum values in window
      unsigned int inputMax = 0;
      unsigned int inputMin = 1024;

      // loop for the window
      for (unsigned int i = 0; i < inputWindow; i++) {
        // read in a single value
        inputSample = analogRead(inputPin);
        // get the minimum and maximum value
        inputMin = min(inputMin, inputSample);
        inputMax = max(inputMax, inputSample);
        diference = inputMax - inputMin;
        if (diference >= maxDiference) {
          maxDiference = diference;
        }
      }

      int porcentaje = (diference * 100) / inputMin;

      encenderPixels(porcentaje);

      ultimoPorcentaje = porcentaje;

      lcd.home ();
      lcd.print(maxDiference);
    }
  }
}

void encenderPixels(int porcentaje) {
  if (porcentaje < ultimoPorcentaje) {
    apagarPixels(ultimoPorcentaje, porcentaje);
    ultimoPorcentaje = 0;
  }

  uint32_t rojo      = Adafruit_NeoPixel::Color(255,  0,   0);
  uint32_t verde     = Adafruit_NeoPixel::Color(  0, 255,  0);
  uint32_t ambar  = Adafruit_NeoPixel::Color(255, 126,  0);

  // For a set of NeoPixels the first NeoPixel is 0, second is 1, all the way up to the count of pixels minus one.
  for (int i = 0; i < numLeds(porcentaje); i++) {

    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    if (i < numLeds(70)) {
      pixels.setPixelColor(i, verde);
    } else if (i >= numLeds(70) && i < numLeds(90)) {
      pixels.setPixelColor(i, ambar);
    } else {
      pixels.setPixelColor(i, rojo);
    }

    pixels.show(); // This sends the updated pixel color to the hardware.
  }
}


void apagarPixels(int inicio, int fin) {
  for (int i = inicio; i >= fin; i--) {
    pixels.setPixelColor(i, 0);
  }
  pixels.show();
}

// Devuelve la cantidad de leds segun el porcentaje de leds a encender
int numLeds(int porcentaje) {
  return (porcentaje * NUMPIXELS) / 100;
}

void endTone() {
  // notes in the melody:
  int melody[] = {
    NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
  };

  // note durations: 4 = quarter note, 8 = eighth note, etc.:
  int noteDurations[] = {
    4, 8, 8, 4, 4, 4, 4, 4
  };

  // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < 8; thisNote++) {

    // to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(13, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
  }
}

