# 1 "/home/f4deb/Documents/ESP32/VHF-144-NANO/SSB_VHF_F4DEB/SSB_VHF_F4DEB.ino"

//***********************************
// Emetteur GO
//
// 
// Arduino 1.18.13
// si5351.h 2.1.4
// 20/08/23 V1.0
//
//***********************************

# 13 "/home/f4deb/Documents/ESP32/VHF-144-NANO/SSB_VHF_F4DEB/SSB_VHF_F4DEB.ino" 2
# 14 "/home/f4deb/Documents/ESP32/VHF-144-NANO/SSB_VHF_F4DEB/SSB_VHF_F4DEB.ino" 2
# 15 "/home/f4deb/Documents/ESP32/VHF-144-NANO/SSB_VHF_F4DEB/SSB_VHF_F4DEB.ino" 2
//*******************
// Constantesu
//*******************






Si5351 si5351;

//rgb_lcd lcd;

const int colorR = 0;
const int colorG = 255;
const int colorB = 0;


// Constantes
# 50 "/home/f4deb/Documents/ESP32/VHF-144-NANO/SSB_VHF_F4DEB/SSB_VHF_F4DEB.ino"
LCD_I2C lcd(0x27, 16, 2); // Default address of most PCF8574 modules, change according

// Variables
uint64_t frequenceInit = 2255842700; // 2255842700 (2255831666 * 6->-88 dBm) (1933570000 * 7 -> -89dBm)   (1503887777 * 9 -> -77dBm) ( VFO -> -89Dbm)
uint64_t frequence = 0;
uint8_t keyboardCode = 0xFF;
uint32_t OL1;
uint64_t frequencyShift = 10000000/6;

static uint64_t frequenceOut = 144300000;
static char stringUint64SeparatorMille[30];
static char stringUint64Format[30];

int compteur = 0; // Cette variable nous permettra de savoir combien de crans nous avons parcourus sur l'encodeur
                                    // (sachant qu'on comptera dans le sens horaire, et décomptera dans le sens anti-horaire)

int etatPrecedentLigneSW; // Cette variable nous permettra de stocker le dernier état de la ligne SW lu, afin de le comparer à l'actuel
int etatPrecedentLigneCLK;


void setup()
{
  bool i2c_found;

  // Initialisation du port série 
  Serial.begin(115200);

  //Initialisation de la broche de LED
  pinMode(11 /* La LED de statut du PLL du Si5351*/, 0x1); // Définition de la broche de la LED de statut de la PLL
  digitalWrite(11 /* La LED de statut du PLL du Si5351*/, 0);

  //Initialisation du synthétiseur Si5351
  i2c_found = si5351.init((2<<6), 0, 0);
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

    lcd.print("JK-144-V0.04");
    lcd.setCursor(0, 1);
    lcd.print("F4DEB 2024");
    Serial.println("JK-144-V0.04");

    // Query a status update and wait a bit to let the Si5351 populate the
  // status flags correctly.
  si5351.update_status();
  delay(100);

  // Configuration des pins de notre Arduino Nano en "entrées", car elles recevront les signaux du KY-040
    pinMode(2 /* La pin D2 de l'Arduino recevra la ligne SW du module KY-040*/, 0x2); // à remplacer par : pinMode(pinArduinoRaccordementSignalSW, INPUT_PULLUP);
                                                            // si jamais votre module KY-040 n'est pas doté de résistance pull-up, au niveau de SW
    pinMode(4 /* La pin D4 de l'Arduino recevra la ligne DT du module KY-040*/, 0x2);
    pinMode(3 /* La pin D3 de l'Arduino recevra la ligne CLK du module KY-040*/, 0x2);

    // Mémorisation des valeurs initiales, au démarrage du programme
    etatPrecedentLigneSW = digitalRead(2 /* La pin D2 de l'Arduino recevra la ligne SW du module KY-040*/);
    etatPrecedentLigneCLK = digitalRead(3 /* La pin D3 de l'Arduino recevra la ligne CLK du module KY-040*/);

    serialPrintFrequency(frequenceOut);
    LcdPrintFrequency (frequenceOut,9);

    pinMode(5,0x0);
    pinMode(6,0x0);
    pinMode(7,0x0);
    pinMode(8,0x0);

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

  pll_state = (not si5351.dev_status.LOL_A && not si5351.dev_status.LOL_B); // Le statut LOL est plus stable que le LOS  
  if (pll_state) // Le PLL est verrouillé
  {
    digitalWrite(11 /* La LED de statut du PLL du Si5351*/,1);
    timer = millis();
  }
  else if (millis() - timer > 1000) // Le PLL est déverrouillé et le temps de 1 seconde est écoulé
  {
    digitalWrite(11 /* La LED de statut du PLL du Si5351*/,0); // Le statut LOL est plus stable que le LOS
    timer = millis();
  }

  //si5351.set_freq(2255842700, SI5351_CLK0); 
  //setOl1(2255842700);
  setFrequencyOut(frequenceOut);

// Lecture des signaux du KY-040 arrivant sur l'arduino
    int etatActuelDeLaLigneCLK = digitalRead(3 /* La pin D3 de l'Arduino recevra la ligne CLK du module KY-040*/);
    int etatActuelDeLaLigneSW = digitalRead(2 /* La pin D2 de l'Arduino recevra la ligne SW du module KY-040*/);
    int etatActuelDeLaLigneDT = digitalRead(4 /* La pin D4 de l'Arduino recevra la ligne DT du module KY-040*/);

    // *****************************************
    // On regarde si la ligne SW a changé d'état
    // *****************************************
    if(etatActuelDeLaLigneSW != etatPrecedentLigneSW) {

        // Si l'état de SW a changé, alors on mémorise son nouvel état
        etatPrecedentLigneSW = etatActuelDeLaLigneSW;

        // Puis on affiche le nouvel état de SW sur le moniteur série de l'IDE Arduino
        if(etatActuelDeLaLigneSW == 0x0){

        }
        // Petit délai de 5 ms, pour filtrer les éventuels rebonds sur SW
        lcd.blinkOff();
        lcd.cursorOff();
        delay(5);
        serialPrintFrequency(frequenceOut);
        LcdPrintFrequency (frequenceOut,9);
    }

    // ******************************************
    // On regarde si la ligne CLK a changé d'état
    // ******************************************
    if(etatActuelDeLaLigneCLK != etatPrecedentLigneCLK) {

      // On mémorise cet état, pour éviter les doublons
      etatPrecedentLigneCLK = etatActuelDeLaLigneCLK;

      if(etatActuelDeLaLigneCLK == 0x0) {

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
        serialPrintFrequency(frequenceOut);
        LcdPrintFrequency (frequenceOut,9);
      }
    }
}

void keyPrint(void){

  if (!digitalRead(6)) {
    frequencyShift = frequencyShift / 10;
    if (frequencyShift < 100){
      frequencyShift = 100;
    }
    while (!digitalRead(6));
    lcd.cursor();
    lcd.blink();
  }
  if (!digitalRead(5)) {
    frequencyShift = frequencyShift * 10;
    if (frequencyShift > 100000000){
      frequencyShift = 100000000;
    }
    while (!digitalRead(5));
    lcd.cursor();
    lcd.blink();
  }
}

void serialPrintFrequency (uint32_t OL){

}

void LcdPrintFrequency (uint64_t frequency, int format){
  lcd.clear();
  Serial.println("JK-144-V0.04");


  char *texte = uint64Format(frequency, format);
  Serial.println(texte);
  lcd.setCursor(0,0);
  lcd.print(texte);

  char *text1 = uint64SeparatorMille(texte);
  Serial.println(text1);
  lcd.setCursor(0,1);
  lcd.print(text1);
}

void setFrequency (void){
  uint32_t save = frequence;
  frequence = frequence + ((compteur * frequencyShift) );
  if (frequence < 100000 ) {
    frequence = save;
    }
  si5351.set_freq(frequence , SI5351_CLK0);
}

void setOl1(uint64_t Ol1){

  si5351.set_freq(Ol1 , SI5351_CLK0);
}

uint32_t getOl1(void){

}

void setFrequencyOut(uint64_t frequence1){
 //   frequenceOut = (OL1 * 6) - OL2; 13544005638    135370000       2255842700 145537500


 Serial.println(8949000);


  setOl1(((frequence1-8949000)*100)/6);
}

uint64_t getFrequencyOut(void){

}


    //frequenceOut = (OL1 * (OL1_COEFF)) + OL2;




/************************************************
 *  String conversion
 ************************************************/

char* uint64Format(uint64_t frequency, int format){

  // variable de travail
  char stringUint64Format[format+1];

  // calcul de la longueur du nombre transmis
  int n = log10(frequency) + 1; // il faut ajouter 1

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
    Serial.print("Error 0001 : Erreur de formatage Conversion frequence");
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

      car = value[j];
      stringUint64SeparatorMille[l] = car;
      j++;
      l++;
    }
  car = value[j];
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
