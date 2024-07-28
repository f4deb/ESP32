#include <si5351.h>

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
// Constantes
//*******************

#define ERROR_0001 "Error 0001 : Erreur de formatage"

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

#define MESSAGE_INIT "JK-144-V0.03"

#define OL1_COEFF 6

#define OL2 8949438

LCD_I2C lcd(0x27, 16, 2); // Default address of most PCF8574 modules, change according

// Variables
uint64_t frequenceInit = 2255842700;     // 2255842700 (2255831666 * 6->-88 dBm) (1933570000 * 7 -> -89dBm)   (1503887777 * 9 -> -77dBm) ( VFO -> -89Dbm)
uint64_t frequence = 0;
uint8_t keyboardCode = 0xFF;
uint32_t OL1;
uint64_t frequencyShift = 10000000/6;

uint32_t frequenceOut = 144300000;

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

  // Établissement de la fréquence de sortie CLK0
  // Saisie de la fréquence en centièmes de Hz. Exemple: 1000 KHz est 100000000. 
  // Il faut aussi annexer les caractères "ULL" à  la fréquence.
  frequence = frequenceInit;
  
  
  // Query a status update and wait a bit to let the Si5351 populate the
  // status flags correctly.
    si5351.update_status();
   delay(100);
  
  // set up the LCD's number of columns and rows:
    lcd.begin();
    lcd.display();
    lcd.backlight();
    lcd.clear();
 
    lcd.print(MESSAGE_INIT);
    lcd.setCursor(0, 1);
    lcd.print("F4DEB 2024");
 
  
  // Print a message to the LCD.
  //lcd.print("  Emetteur GO  ");

  // Query a status update and wait a bit to let the Si5351 populate the
  // status flags correctly.
  si5351.update_status();
  delay(100);


  // Configuration des pins de notre Arduino Nano en "entrées", car elles recevront les signaux du KY-040
    pinMode(pinArduinoRaccordementSignalSW, INPUT_PULLUP);         // à remplacer par : pinMode(pinArduinoRaccordementSignalSW, INPUT_PULLUP);
                                                            // si jamais votre module KY-040 n'est pas doté de résistance pull-up, au niveau de SW
    pinMode(pinArduinoRaccordementSignalDT, INPUT_PULLUP);
    pinMode(pinArduinoRaccordementSignalCLK, INPUT_PULLUP);

    // Mémorisation des valeurs initiales, au démarrage du programme
    etatPrecedentLigneSW  = digitalRead(pinArduinoRaccordementSignalSW);
    etatPrecedentLigneCLK = digitalRead(pinArduinoRaccordementSignalCLK);

    serialPrintFrequency(OL1);
    LcdPrintFrequency (OL1,6);

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
  keyPrint();
  
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



  //si5351.set_freq(2255842700, SI5351_CLK0); 
  //setOl1(2255842700);
  setFrequencyOut(144300000);
  while(1);



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
        serialPrintFrequency(OL1);  
        LcdPrintFrequency (OL1,6);   
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
            setFrequency();
        }
        else {
            // CLK est identique à DT => cela veut dire que nous comptons dans le sens antihoraire
            // Alors on décrémente le compteur
            compteur = -1;
            setFrequency();
        }
        // Petit délai de 5 ms, pour filtrer les éventuels rebonds sur CLK
        delay(5);
        serialPrintFrequency(OL1);       
        LcdPrintFrequency (OL1,6);
      }
    }
}

void keyPrint(void){

  if (!digitalRead(BP2)) {
    frequencyShift = frequencyShift / 10;
    if (frequencyShift < 100){
      frequencyShift = 100;
    }
    while (!digitalRead(BP2));
    lcd.cursor();
    lcd.blink();
  }  
  if (!digitalRead(BP1)) {
    frequencyShift = frequencyShift * 10;
    if (frequencyShift > 100000000){
      frequencyShift = 100000000;
    }
    while (!digitalRead(BP1));
    lcd.cursor();
    lcd.blink();
  }  
}

void serialPrintFrequency (uint32_t OL){


  Serial.print(F("DDS local = "));
  frequence = frequence; 
  OL1 = frequence / 100;
  Serial.print( OL1 );
  Serial.println(F("Hz"));

  lcd.clear();
  lcd.print("OL1:");
  lcd.print((OL1));// * 6) + OL2);
  lcd.setCursor(0, 1);  
  //lcd.print("OL1:");

  
}

void uint64Format(){

}

void uint64SeparatorMille(){

}

void LcdPrintFrequency (uint64_t frequency, int format){


  frequency = 1455375;
  format = 9;

  // calcul de la longueur du nombre transmis
  int n = log10(frequency) + 1;   // il faut ajouter 1

  if (format > n) {

    // Formatage
    char value[format]; 
    sprintf(value, "%ld", frequency);

    // calcul nombre caratere vide 
    int nombreDeZero = format - n;

    // variable de travail
    char tableau[format+1];

    // insertion de caractere vide devant le nombre
    for (int i = 0; i <= nombreDeZero; i++){
      tableau [i] = ' ';
    }

    //transfert du nombre dans le tableau
    for (int i = nombreDeZero; i <= format; i++){
      tableau[i] = value[i-nombreDeZero];
      Serial.print(" i= ");
      Serial.print(i);
    }
    // ajout fin de chaine de caractere
    tableau[n+nombreDeZero] = '\0';
      lcd.print(tableau);
  }
  else {
    Serial.print(ERROR_0001);
  }




/*
  char *car;
  char value1[n+3];
  
  int j = 0;
  int k = 0;
  int p = strlen(value);
  int l = -1;
  for (j = 0; j <= p; j++){
  l++;
    for (k = 0 ; k<2; k++){

      car  = value[j];
      value1[l] = car;
//      if (nombreZero > 0) {
//        value1[l] = "e";
//        nombreZero--;
//        j++;
//      }
      j++;
      l++;

    }
  car  = value[j];
  value1[l] = car;
  l++;
  if (l != p+(p/4)) 
  value1[l] = '.';
  }
  
    lcd.print(value1);
    Serial.print(value1);

  //lcd.print(value);
  lcd.setCursor( 4,  1);
*/
}




void setFrequency (void){
  uint32_t save = frequence;
  frequence = frequence + ((compteur * frequencyShift) );
  if (frequence < 100000 ) {
    frequence = save;
    }
  si5351.set_freq(frequence , SI5351_CLK0);
}

void setOl1(uint32_t Ol1){
  si5351.set_freq(Ol1 , SI5351_CLK0);
}

uint32_t getOl1(void){
  
}

void setFrequencyOut(uint64_t frequence1){
 //   frequenceOut = (OL1 * 6) + OL2; 13544005638    135370000
  setOl1((13544005638-OL2)/6);

}

uint64_t getFrequencyOut(void){

}


    //frequenceOut = (OL1 * (OL1_COEFF)) + OL2;
