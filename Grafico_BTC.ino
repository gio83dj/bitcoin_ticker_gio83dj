#include "Arduino.h" // Inclusione della libreria di Arduino
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include "TFT_eSPI.h" // Inclusione della libreria TFT_eSPI per il display
#include <WiFi.h> // Inclusione della libreria WiFi
#include "time.h" // Inclusione della libreria time per la gestione del tempo
#include "btc_png.h"
#include "background.h"

const char* ssid     = "SSID_NAME"; // SSID della rete Wi-Fi
const char* password = "SSID_PASSWORD"; // Password della rete Wi-Fi

const char* webSocketUrl = "stream.binance.com";
const int webSocketPort = 9443;
const char* webSocketPath = "/ws/btcusdt@kline_5m";

WebSocketsClient webSocket;
WiFiClient wifiClient;

const char* ntpServer = "pool.ntp.org"; // Server NTP per ottenere l'ora corrente
const long  gmtOffset_sec =3600; // Offset rispetto al tempo UTC
const int   daylightOffset_sec = 3600; // Offset per l'ora legale
char timeHour[3]="00"; // Buffer per l'ora
char timeMin[3]="00"; // Buffer per i minuti
char timeSec[3]; // Buffer per i secondi
char m[12]; // Buffer per il mese
char y[5]; // Buffer per l'anno
char d[3]; // Buffer per il giorno
char dw[12]; // Buffer per il giorno della settimana
float lastPrice = 0.0;
float maxValue = 0.0;
float minValue = 0.0;
int gw = 204; // Larghezza del grafico
int gh = 102; // Altezza del grafico
int gx = 80; // Posizione x del grafico
int gy = 160; // Posizione y del grafico

#define gray 0x6B6D // Colore grigio
#define blue 0x0967 // Colore blu
#define orange 0xC260 // Colore arancione
#define purple 0x604D // Colore viola
#define green 0x1AE9 // Colore verde
// Battery.
uint32_t    uVolt;   
int deb=0; // Flag per il debounce
int Mode=0; // Modalità di funzionamento (0 per la modalità random, 1 per la modalità sensore)
#define RIGHT 14 // Pin per il tasto destro
// Definizione della struttura per memorizzare timestamp e valore
struct DataPoint {
  String timeString; // Cambiato da uint64_t a String
  float value;
};
// Dichiarazione e inizializzazione dell'array
const int ARRAY_SIZE = 12; // Modifica la dimensione dell'array se necessario
DataPoint dataPoints[ARRAY_SIZE];
int currentIndex = 0;
int counter=0; // Contatore
long lastMillis=0; // Ultimo tempo di millis
int fps=0; // Frame per secondo

// Dichiarazione e inizializzazione dell'array di supporto per i valori normalizzati
float normalizedValues[ARRAY_SIZE];
int x = 200;
int btcX = 20;
int btcY = 50;
int backgroundX = -10;
int backgroundY = 0;
bool reverse = false;


// SPRITES
TFT_eSPI tft = TFT_eSPI(); // Inizializzazione dell'oggetto TFT per il display
TFT_eSprite background = TFT_eSprite(&tft); // Inizializzazione dell'oggetto sprite per il display
TFT_eSprite arrowSprite= TFT_eSprite(&tft);
TFT_eSprite btcSprite= TFT_eSprite(&tft);




void addDataPoint(uint64_t timestamp, float value) {
  // Se l'array è pieno, sposta tutti i valori di un posto verso sinistra per fare spazio al nuovo valore
  if (currentIndex >= ARRAY_SIZE) {
    for (int i = 1; i < ARRAY_SIZE; i++) {
      dataPoints[i - 1] = dataPoints[i];
    }
    currentIndex = ARRAY_SIZE - 1;
  }

  // Aggiungi il nuovo valore all'ultimo posto disponibile
  DataPoint newDataPoint;
  newDataPoint.timeString = formatTimestamp(timestamp); // Converti il timestamp in stringa
  newDataPoint.value = value;
  dataPoints[currentIndex] = newDataPoint;
  currentIndex++;
}
void printDataPoints() {
  Serial.println("Array di dati:");
  for (int i = 0; i < currentIndex; i++) {
    Serial.print("Timestamp ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(dataPoints[i].timeString);
    Serial.print("Valore ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(dataPoints[i].value);
  }
}
void normalizeDataPoints() {
  // Trova il valore massimo e il valore minimo nell'array
  maxValue = dataPoints[0].value;
  minValue = dataPoints[0].value;
  for (int i = 1; i < currentIndex; i++) {
    if (dataPoints[i].value > maxValue) {
      maxValue = dataPoints[i].value;
    }
    if (dataPoints[i].value < minValue) {
      minValue = dataPoints[i].value;
    }
  }

  // Normalizza i valori nell'intervallo 0-102 nell'array di supporto
  for (int i = 0; i < currentIndex; i++) {
    normalizedValues[i] = map(dataPoints[i].value, minValue, maxValue, 0, gh);
  }
}
// Funzione per stampare i dati normalizzati
void printNormalizedValues() {
  Serial.println("Array di dati normalizzati:");
  for (int i = 0; i < currentIndex; i++) {
    Serial.print("Valore normalizzato ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(normalizedValues[i]);
  }
}
// Funzione per stampare l'ora locale
void printLocalTime(){
  struct tm timeinfo; // Struttura per le informazioni sul tempo
  if(!getLocalTime(&timeinfo)){ // Ottenimento dell'ora locale
    Serial.println("Failed to obtain time"); // Stampa di errore in caso di fallimento
    return;
  }
  strftime(timeHour,3, "%H", &timeinfo); // Formattazione dell'ora
  strftime(timeMin,3, "%M", &timeinfo); // Formattazione dei minuti
  strftime(timeSec,3, "%S", &timeinfo); // Formattazione dei secondi
  strftime(y,5, "%Y", &timeinfo); // Formattazione dell'anno
  strftime(m,12, "%B", &timeinfo); // Formattazione del mese
  strftime(dw,10, "%A", &timeinfo); // Formattazione del giorno della settimana
  strftime(d,3, "%d", &timeinfo); // Formattazione del giorno
}
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  uint64_t timestamp;
  float valore;
  unsigned long seconds, minutes, hours;

  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED:
      Serial.printf("[WSc] Connected to url: %s\n", payload);
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] get text: %s\n", payload);
      // Parse the JSON string received from WebSocket
      DynamicJsonDocument jsonDocument(1024);
      deserializeJson(jsonDocument, payload, length);
      
      // Visualizza il JSON su Serial
      //Serial.println("Contenuto del JSON:");
      //serializeJsonPretty(jsonDocument, Serial);
      //Serial.println();

      // Estrarre i valori desiderati
      JsonObject kline = jsonDocument["k"];
      timestamp = jsonDocument["E"];
      String pair = jsonDocument["s"];
      valore = kline["c"];
      lastPrice = valore;

      // Print the extracted values
      //Serial.print("Timestamp: ");      Serial.println(timestamp);      Serial.print("Valore: ");      Serial.println(valore);      Serial.print("Pair: ");      Serial.println(pair);
      String timeString = formatTimestamp(timestamp);
      Serial.print("Orario: ");
      Serial.println(timeString);
      
      // Aggiungi il nuovo valore all'array
      addDataPoint(timestamp, valore);

      // Chiamare normalizeDataPoints solo se l'array è pieno
      if (currentIndex == ARRAY_SIZE) {
        // Normalizza i valori nell'array originale e memorizzali nell'array di supporto
        normalizeDataPoints();
        // Stampa i dati normalizzati
        //printNormalizedValues();
      }
      printDataPoints();
      break;
  }
}
String formatTimestamp(uint64_t timestamp) {
  unsigned long seconds = (timestamp / 1000) % 60;
  unsigned long minutes = (timestamp / (1000 * 60)) % 60;
  unsigned long hours = (timestamp / (1000 * 60 * 60)) % 24;

  // Costruisci la stringa formattata
  String formattedTime = "";
  formattedTime += hours;
  formattedTime += ":";
  if (minutes < 10) formattedTime += "0";
  formattedTime += minutes;
  formattedTime += ":";
  if (seconds < 10) formattedTime += "0";
  formattedTime += seconds;

  return formattedTime;
}
// Inizializzazione dello sketch
void setup(void)
{
  pinMode(15,OUTPUT); // Impostazione del pin 15 come output
  digitalWrite(15,1); // Attivazione del pin 15
  pinMode(RIGHT,INPUT_PULLUP); // Impostazione del pin RIGHT come input con pull-up
  Serial.begin(115200); // Inizializzazione della comunicazione seriale a 115200 bps


  // SPRITES
  tft.init(); // Inizializzazione del display
  tft.fillScreen(TFT_BLACK); // Riempimento dello schermo con il colore nero
  tft.setRotation(1); // Impostazione della rotazione del display
  background.createSprite(640, 170); // Creazione di uno sprite per il display
  background.setTextDatum(3); // Impostazione del datum per il testo
  background.setSwapBytes(true); // Abilitazione dello swap dei byte
  arrowSprite.createSprite(96,96);
  
  btcSprite.createSprite(50,50); 



  // Connessione alla rete Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  
  // Configurazione del tempo tramite il server NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); 
  analogReadResolution(10); // Impostazione della risoluzione di lettura analogica
  webSocket.beginSSL(webSocketUrl, webSocketPort, webSocketPath);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

// Loop principale dello sketch
void loop()
{
  // Controllo del tasto destro per cambiare modalità
  if(digitalRead(RIGHT)==0){
    if(deb==0)
    {
      Mode++;
      if(Mode==2)
        Mode=0;
        deb=1;
    }
  }else{deb=0;}
  
  fps=1000/(millis()-lastMillis); // Calcolo dei frame per secondo
  lastMillis=millis(); // Aggiornamento dell'ultimo tempo di millis
  //average=0; // Reset del valore medio
  
  if(counter==0) // Aggiornamento dell'ora locale ogni 50 cicli
    printLocalTime();

  counter++;
  if(counter==25)
    counter=0;

  // Scelta della modalità di funzionamento
  if(Mode==1){}
  if(Mode==0){}

  // SPRITES
  // IMMAGINE DI SFONDO
  background.pushImage(backgroundX, backgroundY, 640, 170, backgroundImage); // Riempimento dello sprite con immagine PNG
  //background.fillSprite(TFT_BLACK); // Riempimento dello sprite con il colore nero
  // SPRITE IN MOVIMENTO. Messo in questo punto di codice verrà posto nel livello dietro al grafico
  btcSprite.pushImage(0, 0, 50, 50, btc_50px);
  btcSprite.pushToSprite(&background, btcX, btcY, TFT_BLACK); 


  // Rettangolo Arrotondato Ore (x, y, w, h, arrotondamentoAngoli, colore)
  background.fillRoundRect(6, 5, 38, 32, 4, blue);
  // Rettangolo Arrotondato Minuti
  background.fillRoundRect(50, 5, 38, 32, 4, blue);
  // Rettangolo Arrotondato Secondi
  background.fillRoundRect(90, 5, 20, 20, 4, blue);
  // Rettangolo Arrotondato MESE
  background.fillRoundRect(6, 40, 80, 12, 4, blue);

  // Disegno del colore dello sprite testo (colTesto, colSfondo)
  background.setTextColor(TFT_WHITE, blue);
  // Hh 
  background.drawString(String(timeHour), 10, 24, 4);
  // Min
  background.drawString(String(timeMin), 56, 24, 4);
  // Sec
  background.drawString(String(timeSec), 93, 15, 2);
  // Mese
  background.drawString(String(m) + " " + String(d), 10, 45);
  
  // BTC PRICE
  background.setTextColor(TFT_YELLOW);
  background.drawString("BTC PRICE: ", 120, 6);
  background.drawString(String(lastPrice) + " $", 130, 26, 4);
  //background.drawString("ON PIN 12",gx+10,30);
  background.setFreeFont();

  // Disegno delle 6 linee orizzontali del grafico e relative etichette
  for(int i=1;i<8;i++){
    background.drawLine(gx, gy - ((i - 1) * 17), gx + gw - 17, gy - ((i - 1) * 17), gray); // Disegna una linea orizzontale
    float range = (maxValue - minValue) / 6;
    background.drawString(String(minValue + (range * (i - 1))), gx - 50, gy - ((i - 1) * 17)); // Aggiunge etichette sull'asse Y maxValue
  }

  // Disegno degli assi del grafico
  background.drawLine(gx,gy,gx+gw,gy,TFT_WHITE); // Linea orizzontale per l'asse X
  background.drawLine(gx,gy,gx,gy-gh,TFT_WHITE); // Linea verticale per l'asse Y

  // Disegno del grafico vero e proprio
  if (currentIndex == ARRAY_SIZE) {
    // Disegno del grafico vero e proprio
    for(int i=0;i<(ARRAY_SIZE-1);i++){
      // Disegna una linea rossa che collega ciascun punto del grafico
      background.drawLine(gx + (i * 17), gy - normalizedValues[i], gx + ((i + 1) * 17), gy - normalizedValues[i + 1], TFT_RED);
      //background.fillCircle(gx + (i * 17), gy - normalizedValues[i], gx + ((i + 1) * 17), gy - normalizedValues[i + 1], 7, TFT_YELLOW);
      background.drawLine(gx + (i * 17), gy - normalizedValues[i] - 1, gx + ((i + 1) * 17), gy - normalizedValues[i + 1] - 1, TFT_RED);
      background.fillCircle(gx + ((i + 1) * 17), gy - normalizedValues[i + 1] - 1, 3, TFT_RED);
    }
    background.drawCircle(gx + ((ARRAY_SIZE-1) * 17), gy - normalizedValues[ARRAY_SIZE-1] - 1, 8, TFT_YELLOW);
  }else{
    background.drawString(String("Waiting for data..."), gx + 10, gy - 50, 4);
  }

  // Visualizzazione informazioni BATTERIA
  background.setTextColor(TFT_WHITE);
  //background.drawString("BAT:"+String(analogRead(4)),gx+160,16); // Visualizza il valore dell'ingresso analogico 4
  // Read the battery voltage.
  uVolt = ((analogRead(4) * 2 * 3.3 * 1000) / 4096) * 3.3;
  // Update the battery sprite.
  background.drawString(String(uVolt / 1000) + "." + String(uVolt % 1000) + " vDC", 265, 6); 
  //background.drawString("MOD:"+String(Mode),gx+160,26); // Visualizza la modalità di funzionamento
  
  btcX++;
  if(btcX > 330){
    btcY = random (0, 170);
    btcX = -50;
  }
  
  backgroundX--;
  //Serial.println(backgroundX);
  if(backgroundX < -320){
    backgroundX = 0;
  }
/*
// Il counter è settato a 25 cicli, quindi ogni 25 cicli circa un secondo fa la routine 
  //if(counter==0){
    if (reverse){
      backgroundX--;
      //gy--;
      if(backgroundX < -10){
        reverse = false;
      }   
    } else {
      backgroundX++;
      //gy++;
      if(backgroundX > 10){
        reverse = true;
      }  
    }
  //}
  */
  if(counter==0 || counter == 5){
    background.drawCircle(gx + ((ARRAY_SIZE-1) * 17), gy - normalizedValues[ARRAY_SIZE-1] - 1, 12, TFT_WHITE);
  }
  // Aggiornamento del display
  background.pushSprite(0,0); // Disegna lo sprite sul display

  webSocket.loop();
  //delay(1000); // Aggiungi un piccolo ritardo per evitare il flooding della seriale
} 

