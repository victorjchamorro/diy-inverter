//Librerías
#include <LiquidCrystal.h>

// Pines
#define AVB A0  //Sensor Volt Batería
#define AT1 A1  //Sensor Temperatura 1
#define AT2 A2  //Sensor Temperatura 2
#define AT3 A3  //Sensor temperatura 3
#define DSD 13  //Salida digital de stop para el pwm de la 1ª etapa
#define DSPWM 3 //Salida digital de start para el generador spwm de la 2ª etapa
#define DFAN 5  //Salida digital para el ventilador

// Conf
#define MUESTRAS 100
#define DIVISOR1 3.7

// Debug
bool serial = false;

// Volt
int ADCvRef=0;
float voltBatt=0;
float atenuacion = 1.027;
float voltMax = 15.5;           //Voltaje Máximo
float voltMin = 10.5;           //Voltaje Mínimo
float voltMinRestart = 12.0;    //Voltaje recuperación tras voltaje mínimo
float voltMaxRestart = 14.0;    //Voltaje recuperación tras voltaje máximo

// Temp
float temp1 = 0;
float temp2 = 0;
float temp3 = 0;

float tempMax = 55.0;           //Temp Máxima
float tempMaxRestart = 40.0;    //Temp recuperación tras voltaje máximo
float tempStopFan = 34.0;       //Temp apagado ventilador
float tempStartFan = 40.0;      //Temp encendido ventilador


// Tiempos y control
unsigned long tError = 0;   //milisegundo en el cuall ocurrió el error de voltaje
int tErrorDuration = 4000;  //milisegundos mínimos que durará el estado de error (para evitar rebotes)
bool error = false;         //Estado pwm parado por error
bool fanOn = false;         //Estado de encendido del ventilador

int pasoTemp = 1;

int estadoIcono = 0;

// Manejo de la pantalla LCD
LiquidCrystal lcd(7, 8, 9, 10, 11 , 12);

//Setup
void setup() {
  
  if (serial) Serial.begin(9600);

  pinMode(DFAN, OUTPUT);
  pinMode(DSD,  OUTPUT);
  pinMode(DSPWM,OUTPUT);

  initLCD();

  delay(1500);               //Esperamos para que el voltaje del bus de alta esté casi en nominal (380DC)
  
  statusRun();
  
}

// Resetea el error y limpia los tiempos de error
void resetError(){
  tError=0;
  statusRun();
}

// Para las dos etapas y marca el estado del inversor como error
void statusError(){
  error = true;
  digitalWrite(DSD,HIGH);
  digitalWrite(DSPWM,LOW);
}

// Arranca las 2 etapas y quita el estado de error
void statusRun(){
  error = false;
  
  //Arrancamos la 1ª etapa para que cuando arranque el generador spwm de la 2ª etapa el bus de alta ya esté cercano a los 380v DC
  digitalWrite(DSD,LOW);    //Arrancamos el generador PWM de la primera etapa
  delay(500);               //Esperamos para que el voltaje del bus de alta esté casi en nominal (380DC)
  digitalWrite(DSPWM,HIGH); //Arrancamos el generador SPWM de la segunda etapa
}

// Inicializa la pantalla LCD y pinta los textos que no cambian
void initLCD(){
  
  lcd.begin(16, 2);

  /*byte customChar1[] = {
        B00111,
        B01111,
        B11111,
        B11111,
        B11001,
        B11001,
        B11110,
        B11110
      };
  
    byte customChar2[] = {
        B11100,
        B11110,
        B11111,
        B11111,
        B10011,
        B10011,
        B01111,
        B01111
      };
      
    byte customChar3[] = {
        B11111,
        B01111,
        B00101,
        B00010,
        B00111,
        B00111,
        B00000,
        B00000
      };
      
    byte customChar4[] = {
        B11111,
        B11110,
        B10100,
        B01000,
        B11100,
        B11100,
        B00000,
        B00000
      };

      lcd.createChar(0, customChar1);
      lcd.createChar(1, customChar2);
      lcd.createChar(2, customChar3);
      lcd.createChar(3, customChar4);

    byte customChar5[] = {
          B11000,
          B10000,
          B00000,
          B00000,
          B00110,
          B00110,
          B00001,
          B00001
        };
    
      byte customChar6[] = {
          B00011,
          B00001,
          B00000,
          B00000,
          B01100,
          B01100,
          B10000,
          B10000
        };
        
      byte customChar7[] = {
          B00000,
          B10000,
          B11010,
          B11101,
          B11000,
          B11000,
          B11111,
          B11111
        };
        
      byte customChar8[] = {
          B00000,
          B00001,
          B01011,
          B10111,
          B00011,
          B00011,
          B11111,
          B11111
        };

        lcd.createChar(4, customChar5);
        lcd.createChar(5, customChar6);
        lcd.createChar(6, customChar7);
        lcd.createChar(7, customChar8);
    
  
  /*  // Test Caracteres
      // LCD
      for(int i=0; i<=255; i++){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(i);
        lcd.setCursor(5,0);
        lcd.print(i, BIN);
        lcd.setCursor(0,1);
        byte z = (int) i;;
        lcd.write(z);
        delay(500);
      }
  */
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.write("Batt: ");
  lcd.setCursor(0,1);
  lcd.write("Temp  :");
}

// Actualiza la pantalla LCD
void printInfo(){
  if (serial){
    if (error){
      Serial.println("ERROR: Voltaje o temperatura fuera de rango");
    }
    //Serial.print("vRefADC: ");
    //Serial.println(ADCvRef/1000.0/atenuacion);
    Serial.print("vBat: ");
    Serial.println(voltBatt);
    Serial.print("Temp  :");
    Serial.println(temp1);
  }

  if (pasoTemp == 1){
    initLCD();
  }
  
  

  lcd.setCursor(10,0);
  lcd.print(round(voltBatt*100)/100.0,DEC);
  lcd.setCursor(15,0);
  lcd.write("V");

  //Temp 1;

  switch(pasoTemp){
    case 1:
      pasoTemp = 2;
      lcd.setCursor(5,1);
      lcd.print("1");
      lcd.setCursor(10,1);
      lcd.print(round(temp1*100)/100.0,DEC);
      lcd.setCursor(15,1);
      lcd.write(223); //Carácter º (Grados)
      break;
    case 2:
      pasoTemp = 3;
      lcd.setCursor(5,1);
      lcd.print("2");
      lcd.setCursor(10,1);
      lcd.print(round(temp2*100)/100.0,DEC);
      lcd.setCursor(15,1);
      lcd.write(223); //Carácter º (Grados)
      break;
    case 3:
      pasoTemp = 1;
      lcd.setCursor(5,1);
      lcd.print("3");
      lcd.setCursor(10,1);
      lcd.print(round(temp3*100)/100.0,DEC);
      lcd.setCursor(15,1);
      lcd.write(223); //Carácter º (Grados)
      break;
  }

  if (error){
    if (estadoIcono<1){

        estadoIcono=estadoIcono+1;
  
        lcd.setCursor(7,0);
        lcd.print("FA");
        lcd.setCursor(7,1);
        lcd.print("IL");
        /*lcd.write(byte(0));
        lcd.setCursor(8,0);
        lcd.write(byte(1));
        lcd.setCursor(7,1);
        lcd.write(byte(2));
        lcd.setCursor(8,1);
        lcd.write(byte(3));
          */
          
    }else{
  
      estadoIcono=estadoIcono+1;
      if (estadoIcono>1){
        estadoIcono=0;
      }
      lcd.setCursor(7,0);
      lcd.print("IL");
      lcd.setCursor(7,1);
      lcd.print("FA");
      /*
      lcd.setCursor(7,0);
      lcd.write(byte(4));
      lcd.setCursor(8,0);
      lcd.write(byte(5));
      lcd.setCursor(7,1);
      lcd.write(byte(6));
      lcd.setCursor(8,1);
      lcd.write(byte(7));
      */
    }

/*
    
    lcd.setCursor(7,0);
    lcd.write(byte(0));
    lcd.setCursor(8,0);
    lcd.write(byte(1));
    lcd.setCursor(7,1);
    lcd.write(byte(2));
    lcd.setCursor(8,1);
    lcd.write(byte(3));
    */
  }else{
    lcd.setCursor(7,0);
    lcd.write(" ");
    lcd.setCursor(8,0);
    lcd.write(" ");
    lcd.setCursor(7,1);
    lcd.write(" ");
    lcd.setCursor(8,1);
    lcd.write(" ");
  }
}

// Devuelve true si el voltaje de batería es bajo
bool checkVoltMin(){
  bool minerror = (voltBatt<voltMin);
  if (minerror){
    tError = millis();
  }
  return minerror;
}

// Devuelve true si el voltaje de batería es elevado
bool checkVoltMax(){
  bool maxerror = (voltBatt>voltMax);
  if (maxerror){
    tError = millis();
  }
  return maxerror;
}

// Devuelve true si la temperatura es elevada
bool checkTempMax(){
  bool maxerror = (temp1>tempMax) || (temp2>tempMax) || (temp3>tempMax);
  if (maxerror){
    tError = millis();
  }
  return maxerror;
}

//Control del Ventilador
bool checkVentilador(){
  if (fanOn == true){
    //Miro si tengo que apagar
    if(temp1<tempStopFan && temp2<tempStopFan && temp3<tempStopFan){
      //digitalWrite(DFAN,LOW);
      analogWrite(DFAN,0);
      fanOn = false;
    }
  }else{
    //Miro si tengo que encender
    if(temp1>tempStartFan || temp2>tempStartFan || temp3>tempStartFan){
      analogWrite(DFAN,170); //Usamos pwm para ajustar a 12v en lugar de los 16 que tenemos
      fanOn = true;
    }
  }
  
}


/**
 * Voltaje de Bateria en voltios
 */
float vBatt(){
  double volts = 0;
  
  for(int i=0;i < MUESTRAS; i++){    
    volts = volts + ((analogRead(AVB) / 1023.0) * ADCvRef);
    delay(1);
  }
  return (((volts / MUESTRAS) / 1000) / atenuacion)*DIVISOR1;
}


/**
 * Temperatura de la sonda en el pin indicado en ºC
 */
float cTemp(int pin){
  //Medimos voltaje del divisor de tensión
  double volts = 0;
  
  for(int i=0;i < MUESTRAS; i++){    
    volts = volts + ((analogRead(pin) / 1023.0) * ADCvRef);
    delay(1);
  }
  double V = (((volts / MUESTRAS) / 1000.0) / atenuacion)*1.0;

  //Curva logarítmica obtenida con la hoja de cálculo
  return -34.4456*log(V) + 56.68735;
}

/**
 * Devuelve el voltaje de referencia ~5v ó ~3.3v en milivoltios
 */
int vRefADC(){
   long result;
   ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
   delay(2);
   ADCSRA |= _BV(ADSC);
   while (bit_is_set(ADCSRA,ADSC));
   result = ADCL;
   result |= ADCH<<8;
   result = 1125300L / result;
   return result;
}

void loop() {
  // put your main code here, to run repeatedly:

  ADCvRef = vRefADC();
  voltBatt= vBatt();
  temp1   = cTemp(AT1);
  temp2   = cTemp(AT2);
  temp3   = cTemp(AT3);

  checkVentilador();

  if (error == true){
    unsigned long now = millis();

    if ((tError + tErrorDuration) < now){
      //Ha pasado el tiempo de error, chequeamos si ya estamos en rangos de recuperación
      if(voltBatt>voltMinRestart &&  voltBatt < voltMaxRestart && temp1<tempMaxRestart && temp2<tempMaxRestart && temp3<tempMaxRestart){   
        resetError();
      }else{
        //Volvemos a resetear el tiempo de error para que vuelva a esperar
        tError=now;
      }
    }
    
  }
  
  if (error==false){
    //Si no estamos en error, revisamos los parámetros
    
    //Voltajes:
    if (error == false && (checkVoltMin() || checkVoltMax())){
      statusError();
    }

    //Temperatura:
    if (error == false && checkTempMax()){
      statusError();
    }
  
  }

  printInfo();
  delay(500);
}
