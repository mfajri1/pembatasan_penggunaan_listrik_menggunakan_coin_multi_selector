#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Keypad.h>
#include <SoftwareSerial.h>
#include <PZEM004Tv30.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Servo.h>
#include <HX711.h> //memasukan library HX711
#include <DFPlayer_Mini_Mp3.h>

#define Buzzer 6
#define RST_PIN         5 
#define SS_PIN          53
int R = 34;
int G = 35;
int B = 36;

SoftwareSerial mp33(13,12);
//konfigurasi hx771
#define DOUT  32 //mendefinisikan pin arduino yang terhubung dengan pin DT module HX711
#define CLK  33 //mendefinisikan pin arduino yang terhubung dengan pin SCK module HX711
HX711 scale(DOUT, CLK);
float calibration_factor = 483;

PZEM004Tv30 pzem(10, 11); // Software Serial pin 11 (RX) & 12 (TX)
//konfig rfid
MFRC522 mfrc522(SS_PIN, RST_PIN);

String card1 = "24313518926";
String card2 = "11516420849";

// inisiasi servo
Servo servo1;

const byte ROWS = 4; //four rows
const byte COLS = 4; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3', 'a'},
  {'4','5','6', 'b'},
  {'7','8','9', 'c'},
  {'*','0','#', 'd'}
};
byte rowPins[ROWS] = {22, 24, 26, 28}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {23, 25, 27, 29}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

//konfig lcd
LiquidCrystal_I2C lcd(0x27, 16, 2);

int relay = 7;
long int angka = 0;
long int daya = 0;
String modet = "";
String stat = "";
String IDTAG = "";
int coinsignal = 0;
unsigned long rupiah = 0;
int nilaiRupiah = 0;
int i = 0;
int jmlKoin = 0;
float unit = 0;
float voltage, power, current, nilaiPower;
void setup() {
  Serial.begin(115200);
  mp33.begin(9600);
  SPI.begin();
  pinMode(R, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(B, OUTPUT);
  pinMode(3, INPUT_PULLUP);
  pinMode(Buzzer, OUTPUT);
  pinMode(relay, OUTPUT);
  scale.set_scale();
  scale.tare(); // auto zero / mengenolkan pembacaan berat
  
  long zero_factor = scale.read_average(); //membaca nilai output sensor saat tidak ada beban
  Serial.print("Zero factor: ");
  Serial.println(zero_factor);  
  attachInterrupt(0, tampungsignal, FALLING);
  lcd.begin();
  lcd.backlight(); 
  servo1.attach(8);
  servo1.write(0);
  servo1.detach();
  // setting awal RFID  
  mfrc522.PCD_Init();
  Serial.println("Dekatkan Kartu RFID Anda ke Reader");
  Serial.println();
  mp3_set_serial (mp33);
  delay(10);
  mp3_set_volume (35);
  delay(10);
  mp3_play (01);
  analogWrite(R, 255);
  analogWrite(G, 255);
  analogWrite(B, 255);
  tampilLcd("Menu1=Transaksi", "Menu2=Ambil Koin", "");
  delay(5000);
  tampilLcd("Silahkan Mulai", "Pilih Transaksi", "");
  delay(1000);
}

void loop() {
  analogWrite(R, 0);
  analogWrite(G, 0);
  analogWrite(B, 255);
  scale.set_scale(calibration_factor);
  unit = scale.get_units();
  Serial.println(unit);
  if(unit < 0){
    unit = 0;
  }
  if(unit >= 100){
    mp3_play (02);
    Serial.println(unit);
    tampilLcd("Koin Penuh", "", "");
    delay(2000);
  }
  char key = keypad.getKey();
  switch(key){
    case '1' :
      buzzerTekan();
      modet = "t";//transaksi
      tampilLcd("Mode = Transaksi", "Masukkan Daya", "");
      servo1.detach();
      analogWrite(R, 0);
      analogWrite(G, 255);
      analogWrite(B, 0);
      delay(1000);
    break;
    case '2' :
      analogWrite(R, 0);
      analogWrite(G, 255);
      analogWrite(B, 0);
      buzzerTekan();
      modet = "a";//ambil koin
      tampilLcd("Mode = Ambil Koin", "Tempelkan Kartu", "");
      delay(1000);
    break;
    case '3' :
      modet = "b";//cek berat
      tampilLcd("Mode = Cek Berat", "Berat = ", String(unit));
      delay(1000);
    break;
    case '*' :
      modet = "";//reset mode
      tampilLcd("Pilih Mode", "Transaksi", "");
      delay(1000);
    break;
  }

  if(modet == "t"){
    prosesDaya();
  }else if(modet == "a"){
    tampilLcd("Mode = Ambil Koin", "Tempelkan Kartu", "");
    delay(1000);
    if(! mfrc522.PICC_IsNewCardPresent()){
      return ;
    }
    if(! mfrc522.PICC_ReadCardSerial()){
      return ;
    }
    for(byte i=0; i<mfrc522.uid.size; i++)
    {
      IDTAG += mfrc522.uid.uidByte[i];
    }
    Serial.println(IDTAG);
    
    if(IDTAG != ""){
      if(IDTAG == card1 || IDTAG == card2){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("ID ");
        lcd.print(IDTAG);
        servo1.attach(8);
        servo1.write(160);
        delay(5000);
        servo1.attach(8);
        servo1.write(0);
        tampilLcd("Menu1=Transaksi", "Menu2=Ambil Koin", "");
        delay(2000);
        tampilLcd("Silahkan Mulai", "Pilih Transaksi", "");
        
        
      }else{
        tampilLcd("Tidak Cocok", "", "");
        delay(500); 
        tampilLcd("Menu1=Transaksi", "Menu2=Ambil Koin", "");
        delay(2000);
        tampilLcd("Silahkan Mulai", "Pilih Transaksi", "");
      }
    } 
    delay(1000);
    IDTAG = "";
  }
  //delay(1000);
  modet = "";
  angka = 0;
  

}

void prosesDaya(){
  tampilLcd("Masukkan Daya", "", "");
  stat = "trans";
  daya = ProsesKeypad(stat);
  tampilLcd("Jumlah Daya", "Daya = ", String(daya));
  if(daya > 0 && daya <= 100){
    jmlKoin = 1000;
  }else if(daya >= 101 && daya <= 200){
    jmlKoin = 2000;
  }else if(daya >= 201 && daya <= 300){
    jmlKoin = 3000;
  }else{
    jmlKoin = 0;
    
  }
  delay(1000);
  tampilLcd("Masukkan Koin", "", "");
  Serial.println(jmlKoin);
  coinsignal = 0;
  mp3_play (03);
  while(nilaiRupiah < jmlKoin){
    
    Serial.println(coinsignal);
    switch(coinsignal){
      case 1:
        buzzerTekan();
        rupiah = 10 * 100;
        Serial.println(rupiah);
        coinsignal = 0;  
      break;
      case 2:
        buzzerTekan();
        rupiah = 5 * 100;
        Serial.println(rupiah);
        coinsignal = 0;  
      break;    
      case 3:
        buzzerTekan();
        rupiah = 5 * 100;
        Serial.println(rupiah);
        coinsignal = 0;  
      break;
    }
    
    nilaiRupiah += rupiah;
    
    tampilLcd("Jumlah Koin", "Koin = ", String(nilaiRupiah));
    rupiah = 0; 
    
    delay(1000); 
  }
  tampilLcd("Mulai pemakaian", "", "");
  delay(1000);
  digitalWrite(relay, HIGH);
  delay(3000);
  while(nilaiPower < daya){
    analogWrite(R, 0);
    analogWrite(G, 255);
    analogWrite(B, 0);
    current = pzem.current();
    power = pzem.power();
    voltage = pzem.voltage();
    
    nilaiPower += power;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("V=");
    lcd.print(voltage);
    lcd.print(",A=");
    lcd.print(current);
    lcd.setCursor(0,1);
    lcd.print("W=");
    lcd.print(power);
    lcd.print(",P=");
    lcd.print(nilaiPower);
    float tot = daya - nilaiPower;
    Serial.println(tot);
    if(tot <10.0){
      analogWrite(R, 255);
      analogWrite(G, 0);
      analogWrite(B, 0);
      digitalWrite(Buzzer,HIGH);
      delay(1000);
      digitalWrite(Buzzer, LOW);
      tampilLcd("Daya Hampir Habis", "", "");
      delay(500);
    }
    delay(1000);
    
  }
  digitalWrite(relay, LOW);
  tampilLcd("Menu1=Transaksi", "Menu2=Ambil Koin", "");
  delay(2000);
  tampilLcd("Silahkan Mulai", "Pilih Transaksi", "");
  
  nilaiPower = 0;
  rupiah = 0; 
  jmlKoin = 0;
  daya = 0;
  nilaiRupiah = 0;
  return;
  
}

void prosesAmbilKoin(){
  // Baca Rfid
  
    
//    return;
}


void tampilLcd(String data1, String data2, String data3){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(data1);
  lcd.setCursor(0,1);
  lcd.print(data2);
  lcd.print(data3); 
}

long int ProsesKeypad(String stat){
  while(1){
    char key = keypad.getKey();
    switch(key){
      case '0' ... '9' :
        buzzerTekan();
        angka = angka * 10 + (key - '0');
        if(stat == "trans"){
          tampilLcd("Waktu ", "Jumlah = ", String(angka));  
        }
        
      break;
      case '#' :
          buzzerTekan();
          return angka;
       break;
       case '*' :
        buzzerTekan();
        tampilLcd("", "Data = ", String(angka));  
        angka = 0;
       break;
    }
  }
}

void tampungsignal() {
  coinsignal++;
  i = 0;
}

void buzzerTekan(){
  digitalWrite(Buzzer, HIGH);
  delay(100);
  digitalWrite(Buzzer, LOW);
}
