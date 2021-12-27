#include<Wire.h>
#include <PinChangeInterruptBoards.h>
#include <YetAnotherPcInt.h>

// VCNL4010 I2C címe 0x13
#define Addr 0x13
// Ledek
#define LED_R 5
#define LED_G 10
// Le / Fel irányok
#define LE_IRANY 26 
#define FEL_IRANY 27
// Encoder input értékek
#define MOT1A A8
#define MOT1B A9

// Áltagolt kezdő állapotérték
float defaultLow = 0;
// Encoder állapot
long int count1=0;

void setup()
{
  // I2C kommunikáció inicializálása MASTERként 
  Wire.begin();
  // Soros kommunikáció inicializálása, rate = 9600
  Serial.begin(9600);

  // Input értékek
  pinMode(MOT1A, INPUT);
  pinMode(MOT1B, INPUT);
  
  // Megszakitas bemenetek csatolasa
  PcInt::attachInterrupt(MOT1A, Encoder1A, CHANGE);
  PcInt::attachInterrupt(MOT1B, Encoder1B, CHANGE);

  // Output értékek
  pinMode(LED_R, OUTPUT); // LED_R0
  pinMode(LED_G, OUTPUT); // LED_G

  pinMode(LE_IRANY, OUTPUT); // LED_G
  pinMode(FEL_IRANY, OUTPUT); // LED_G

  // Ledek kezdő értéke kikapcsolt
  digitalWrite(LED_R,LOW); 
  digitalWrite(LED_G,LOW); 

  // Kezdő pozícióba mozgatás (fel fele mozgatás)
  digitalWrite(LE_IRANY,LOW);
  digitalWrite(FEL_IRANY,HIGH);

  // I2C kommunikáció elindítása
  Wire.beginTransmission(Addr);
  // Parancs regiszter kiválasztása
  Wire.write(0x80);
  // Távolság mérés engedélyezése
  Wire.write(0xFF);
  // I2C kommunikáció leállítása
  Wire.endTransmission();

  // I2C kommunikáció elindítása
  Wire.beginTransmission(Addr);
  // Proximity rate register kiválasztása
  Wire.write(0x82);
  // 1.95 távolság mérés/másodperc beállítása
  Wire.write(0x00);
  // I2C kommunikáció leállítása
  Wire.endTransmission();

  //100 iteráción át a távolságnak a kezdeti állapotban átlagot számol
  for (int j = 0; j < 100; j++){
      unsigned int data[2];
    for(int i = 0; i < 2; i++){ 
      //I2C átadás elkezdése
      Wire.beginTransmission(Addr);
      // Adat register kiválasztása
      Wire.write((135+i));
      // I2C átadás leállítása
      Wire.endTransmission();
      //2 byte adat lekérése az eszközről
      Wire.requestFrom(Addr, 1);
      // Távolság msb, Távolság lsb
      if(Wire.available() == 1){
        data[i] = Wire.read();
      }
      //Késleltetés, hogy legyen elég ideje a motornak a kart a kezdő pozícióba állítani
      delay(30);
    }
    //Adott állapot hozzáadása az összegzőhöz
    defaultLow += (data[0] * 256) + data[1];
  }
  
  //Az összegzett érték leosztása az iteráció számával, hogy átlagot kapjunk
  defaultLow /= 100;

  //A kar mozgatásának leállítása
  digitalWrite(LE_IRANY, LOW);
  digitalWrite(FEL_IRANY, LOW);
  delay(300);
  //Az encoder számlálójának alapértékét 1000-re állítjuk,
  //a későbbiekben a kommunikáció által adott késleletétsek, 
  //aminek nem tudunk kontrolálni ne tudjanak negatív értéket adni a számlálónak.
  count1 = 1000;
}

void Encoder1A(void)
{ if (digitalRead(MOT1A) == digitalRead(MOT1B)) count1--;
 else count1++;
}
void Encoder1B(void)
{ if (digitalRead(MOT1A) == digitalRead(MOT1B)) count1++;
 else count1--;
}


void loop(){
  unsigned int data[2];
  for(int i = 0; i < 2; i++){ 
    //I2C átadás elkezdése
    Wire.beginTransmission(Addr);
    // Adat register kiválasztása
    Wire.write((135+i));
    // I2C átadás leállítása
    Wire.endTransmission();

    //2 byte adat lekérése az eszközről
    Wire.requestFrom(Addr, 1);

    // Távolság msb, Távolság lsb
    if(Wire.available() == 1){
      data[i] = Wire.read();
    }
  }

  //Távolság tárolása
  float proximity = ((data[0] * 256) + data[1]);

  //Ledek bekapcsolása
  digitalWrite(5,HIGH);
  digitalWrite(6,HIGH);
  digitalWrite(10,HIGH);

  //Fő logika a karok irányítására
  //count1 => Az encoder állapotát tárolja
  //(proximity - defaultLow)* 5.172 + 1000 
  //=> Távolságból kivonjuk az egyensúlyi állapotban mért átlagos távolságot
  //  5.172-tel megszorozzuk, hogy ugyan abban az intervallumban mozogjon mint az Encoder állapot
  //  1000-et hozzá adunk, hogy összehasonlítható értéket kapjunk az Encoderével, ahol a +1000 ez buffer, hogy negatív értéket ne kaphassunk.                                    
  if(count1 <= ((proximity - defaultLow)* 5.172 + 1000) +500 && count1 >= ((proximity - defaultLow)* 5.172 + 1000) - 500  ){
    digitalWrite(LE_IRANY, LOW);
    digitalWrite(FEL_IRANY, LOW);
  } else if(count1 <= ((proximity - defaultLow)* 5.172 + 1000) +500 && count1 <= ((proximity - defaultLow)* 5.172 + 1000) - 500 && count1 <16000){
    digitalWrite(LE_IRANY, HIGH);
    digitalWrite(FEL_IRANY, LOW);
  } else if(count1 >= ((proximity - defaultLow)* 5.172 + 1000) +500 && count1 >= ((proximity - defaultLow)* 5.172 + 1000) - 500 && count1 >=1000){
    digitalWrite(LE_IRANY, LOW);
    digitalWrite(FEL_IRANY, HIGH);
  }

  //A távolság encoder intervallumra átskálázott értékének kiírása
  Serial.println((proximity - defaultLow)* 5.172 + 1000);

  delay(10);
}
