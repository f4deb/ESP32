
//***********************************
// Emetteur GO
//
// 
// Arduino 1.18.13
// si5351.h 2.1.4
// 20/08/23 V1.0
//
//***********************************

#include "si5351.h"
#include "Wire.h"
#include <LCD-I2C.h>
//*******************
// Constantesu
//*******************

#define ERROR_0001 "Error 0001 : Erreur de formatage Conversion frequence"

#define Ver_Touche 9 // Verrouilage de la roue codeuse

#define PLL_LED 11    // La LED de statut du PLL du Si5351
Si5351 si5351;

//rgb_lcd lcd;

const int colorR = 0;
const int colorG = 255;
const int colorB = 0;


// Constantes
#define pinArduinoRaccordementSignalSW  2       // La pin D2 de l'Arduino recevra la ligne SW du module KY-040
#define pinArduinoRaccordementSignalCLK 3       // La pin D3 de l'Arduino recevra la ligne CLK du module KY-040
#define pinArduinoRaccordementSignalDT  4       // La pin D4 de l'Arduino recevra la ligne DT du module KY-040

#define BP1 5
#define BP2 6
#define BP3 7
#define BP4 8

#define MESSAGE_INIT "JK-144-V0.05"

#define OL1_COEFF 6
#define OL2 8949000
#define FREQUENCE_FORMAT 9


LCD_I2C lcd(0x27, 16, 2); // Default address of most PCF8574 modules, change according



// Variables
uint64_t frequenceInit = 2255842700;     // 2255842700 (2255831666 * 6->-88 dBm) (1933570000 * 7 -> -89dBm)   (1503887777 * 9 -> -77dBm) ( VFO -> -89Dbm)
uint64_t frequence = 0;
uint8_t keyboardCode = 0xFF;
uint32_t OL1;
uint64_t frequencyShift = 100;
int posCursor = log10(frequencyShift) + 1; // il faut rajouter 1;

static uint64_t frequenceOut = 144300000;
static char stringUint64SeparatorMille[30];
static char stringUint64Format[30];

int compteur = 0;                   // Cette variable nous permettra de savoir combien de crans nous avons parcourus sur l'encodeur
                                    // (sachant qu'on comptera dans le sens horaire, et décomptera dans le sens anti-horaire)

int etatPrecedentLigneSW;           // Cette variable nous permettra de stocker le dernier état de la ligne SW lu, afin de le comparer à l'actuel
int etatPrecedentLigneCLK; 


void setup()
{
  bool i2c_found;

  // Initialisation du port série 
  Serial.begin(115200);

  //Initialisation de la broche de LED
  pinMode(PLL_LED, OUTPUT);     // Définition de la broche de la LED de statut de la PLL
  digitalWrite(PLL_LED, 0);

  //Initialisation du synthétiseur Si5351
  i2c_found = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0); 
  si5351.set_pll_input(SI5351_PLLA, SI5351_PLL_INPUT_XO);

  if(!i2c_found)
  {
    Serial.println("Synthétiseur non détecté sur le bus I2C!");
  }

  // Query a status update and wait a bit to let the Si5351 populate the
  // status flags correctly.
    si5351.update_status();
    delay(100);
  
  // set up the LCD's number of columns and rows:
    lcd.begin();
    lcd.display();
    lcd.backlight();
    lcd.blink();
    lcd.clear();
 
    lcd.print(MESSAGE_INIT);
    lcd.setCursor(0,1);
    lcd.print("F4DEB 2024");
    Serial.println(MESSAGE_INIT);
    delay(1000);

    setPosCursorFrequency();


  // Configuration des pins de notre Arduino Nano en "entrées", car elles recevront les signaux du KY-040
    pinMode(pinArduinoRaccordementSignalSW, INPUT_PULLUP);         // à remplacer par : pinMode(pinArduinoRaccordementSignalSW, INPUT_PULLUP);
                                                            // si jamais votre module KY-040 n'est pas doté de résistance pull-up, au niveau de SW
    pinMode(pinArduinoRaccordementSignalDT, INPUT_PULLUP);
    pinMode(pinArduinoRaccordementSignalCLK, INPUT_PULLUP);

    // Mémorisation des valeurs initiales, au démarrage du programme
    etatPrecedentLigneSW  = digitalRead(pinArduinoRaccordementSignalSW);
    etatPrecedentLigneCLK = digitalRead(pinArduinoRaccordementSignalCLK);
   
    serialPrintFrequency(frequenceOut);
    LcdPrintFrequency (frequenceOut,FREQUENCE_FORMAT);

    pinMode(BP1,INPUT);
    pinMode(BP2,INPUT);
    pinMode(BP3,INPUT);    
    pinMode(BP4,INPUT);  

    // Petite pause pour laisser se stabiliser les signaux, avant d'attaquer la boucle loop
    delay(100);
}

void loop()
{
  bool pll_state = false;
  static unsigned long timer = millis();
  
  
  // Read the Status Register and print it every 10 seconds
  si5351.update_status();

  // Établissement de la fréquence de sortie CLK0
  // Saisie de la fréquence en centièmes de Hz. Exemple: 1000 KHz est 100000000. 
  // Il faut aussi annexer les caractères "ULL" à  la fréquence.
  //frequence = frequenceInit + (compteur * shift);
  //si5351.set_freq(frequence , SI5351_CLK0); 
  
  // Détection du signal référence externe pour activer la LED. Lecture directe du registre de statut. 
  // Anti-rebond nécessaire.

  pll_state = (not si5351.dev_status.LOL_A && not si5351.dev_status.LOL_B);  // Le statut LOL est plus stable que le LOS  
  if (pll_state)    // Le PLL est verrouillé
  {
    digitalWrite(PLL_LED,1);
    timer = millis();
  }
  else if (millis() - timer > 1000)  // Le PLL est déverrouillé et le temps de 1 seconde est écoulé
  {
    digitalWrite(PLL_LED,0);  // Le statut LOL est plus stable que le LOS
    timer = millis();
  }

  setFrequencyOut(frequenceOut);

  keyPrint();  

// Lecture des signaux du KY-040 arrivant sur l'arduino
    int etatActuelDeLaLigneCLK = digitalRead(pinArduinoRaccordementSignalCLK);
    int etatActuelDeLaLigneSW  = digitalRead(pinArduinoRaccordementSignalSW);
    int etatActuelDeLaLigneDT  = digitalRead(pinArduinoRaccordementSignalDT);

    // *****************************************
    // On regarde si la ligne SW a changé d'état
    // *****************************************
    if(etatActuelDeLaLigneSW != etatPrecedentLigneSW) {

        // Si l'état de SW a changé, alors on mémorise son nouvel état
        etatPrecedentLigneSW = etatActuelDeLaLigneSW;

        // Puis on affiche le nouvel état de SW sur le moniteur série de l'IDE Arduino
        if(etatActuelDeLaLigneSW == LOW){

        }
        // Petit délai de 5 ms, pour filtrer les éventuels rebonds sur SW
        lcd.blinkOff();
        lcd.cursorOff();
        delay(5);
        serialPrintFrequency(frequenceOut);  
        LcdPrintFrequency (frequenceOut,FREQUENCE_FORMAT);   
    }

    // ******************************************
    // On regarde si la ligne CLK a changé d'état
    // ******************************************
    if(etatActuelDeLaLigneCLK != etatPrecedentLigneCLK) {

      // On mémorise cet état, pour éviter les doublons
      etatPrecedentLigneCLK = etatActuelDeLaLigneCLK;

      if(etatActuelDeLaLigneCLK == LOW) {
        
        // On compare le niveau de la ligne CLK avec celui de la ligne DT
        // --------------------------------------------------------------
        // Nota : - si CLK est différent de DT, alors cela veut dire que nous avons tourné l'encodeur dans le sens horaire
        //        - si CLK est égal à DT, alors cela veut dire que nous avons tourné l'encodeur dans le sens anti-horaire

        if(etatActuelDeLaLigneCLK != etatActuelDeLaLigneDT) {
            // CLK différent de DT => cela veut dire que nous comptons dans le sens horaire
            // Alors on incrémente le compteur
            compteur = 1 ;
            setPosCursorFrequency();
            lcd.setCursor(posCursor,0);
        }
        else {
            // CLK est identique à DT => cela veut dire que nous comptons dans le sens antihoraire
            // Alors on décrémente le compteur
            compteur = -1;
            setPosCursorFrequency();
            lcd.setCursor(posCursor,0);
        }
        // Petit délai de 5 ms, pour filtrer les éventuels rebonds sur CLK
        delay(5);
        frequenceOut = frequenceOut + (compteur*frequencyShift);
        serialPrintFrequency(frequenceOut);       
        LcdPrintFrequency (frequenceOut,FREQUENCE_FORMAT);
      }
    }
}

void setPosCursorFrequency(void){
              posCursor = 11-(log10(frequencyShift) + 1); // il faut rajouter 1
              if (posCursor < 5 ){
                posCursor = posCursor - 2;
              }  
              else if (posCursor < 8 ){
                posCursor = posCursor - 1 ;
              }

}

void keyPrint(void){

  if (!digitalRead(BP2)) {
    frequencyShift = frequencyShift / 10;
    if (frequencyShift < 1){
      frequencyShift = 1;
    }
    while (!digitalRead(BP2));
    lcd.cursor();
    lcd.blink();
    
    setPosCursorFrequency();
    lcd.setCursor(posCursor,0);
    LcdPrintFrequency (frequenceOut,FREQUENCE_FORMAT);
  }  
  if (!digitalRead(BP1)) {
    frequencyShift = frequencyShift * 10;
    if (frequencyShift > 100000000){
      frequencyShift = 100000000;
    }
    while (!digitalRead(BP1));
    lcd.cursor();
    lcd.blink();
    Serial.print("shift : ");
    Serial.println ((long)frequencyShift); 
    
    setPosCursorFrequency();
    lcd.setCursor(posCursor,0);
    LcdPrintFrequency (frequenceOut,FREQUENCE_FORMAT);                     
  } 
}

void serialPrintFrequency (uint32_t OL){
 
}

void LcdPrintFrequency (uint64_t frequency, int format){

  char *texte = uint64Format(frequency, format);
  Serial.println(texte);

  char *text1 = uint64SeparatorMille(texte);
  lcd.setCursor(0,0);
  lcd.print(text1);
  lcd.print("Hz");

  lcd.setCursor(posCursor,0);
}

void setOl1(uint64_t frequence){
  si5351.set_freq(frequence , SI5351_CLK0);
}

void setFrequencyOut(uint64_t frequence){
  setOl1(((frequence-OL2)*100)/OL1_COEFF);
}

/************************************************
 *  String conversion
 ************************************************/

char* uint64Format(uint64_t frequency, int format){

  // variable de travail
  char stringUint64Format[format+1];

  // calcul de la longueur du nombre transmis
  int n = log10(frequency) + 1;   // il faut ajouter 1

  if (format >= n) {

    // Formatage
    char value[format]; 
    sprintf(value, "%ld", frequency);

    // calcul nombre caratere vide 
    int nombreDeZero = format - n;

    // insertion de caractere vide devant le nombre
    for (int i = 0; i <= nombreDeZero; i++){
      stringUint64Format [i] = '0';
    }

    //transfert du nombre dans le stringUint64Format
    for (int i = nombreDeZero; i <= format; i++){
      stringUint64Format[i] = value[i-nombreDeZero];
    }
    // ajout fin de chaine de caractere
    stringUint64Format[n+nombreDeZero] = '\0';
    //lcd.print(tableau);
  }
  else {
    Serial.print(ERROR_0001);
  }

  int adresse = &stringUint64Format;
  return (adresse);
}

char* uint64SeparatorMille(char *value){
  
  char car;
  //int n = strlen(value) ;   // il faut ajouter 1
  int p = strlen(value);

  int j = 0;
  int k = 0;
  int l = 0;

  for (j = 0; j <= p-1; j++){
    for (k = 0 ; k<2; k++){

      car  = value[j];
      stringUint64SeparatorMille[l] = car;
      j++;
      l++;      
    }
  car  = value[j];
  stringUint64SeparatorMille[l] = car;
  l++;
  if (l != p+(p/4)) {
    stringUint64SeparatorMille[l] = '.';
    }
  l++;
  }
  stringUint64SeparatorMille[l-1] = '\0';

  int adresse = &stringUint64SeparatorMille;
  return adresse;
}