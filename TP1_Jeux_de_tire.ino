// ==================================================
// CONFIGURATION DES BROCHES
// ==================================================

const int DATA_PIN = 5;     // D1 - GPIO5
const int CLOCK_PIN = 4;    // D2 - GPIO4
const int LATCH_PIN = 14;   // D5 - GPIO14
const int BUTTON_PIN = 12;  // D6 - GPIO12
const int BUZZER_PIN = 13;  // D7 - GPIO13


// ==================================================
// PARAMETRES DU JEU
// ==================================================

const int MAX_POWER = 5;

const unsigned long CHARGE_INTERVAL = 500;
const unsigned long DEBOUNCE_DELAY = 40;
const unsigned long TARGET_DELAY = 1000;
const unsigned long TARGET_FLASH_DURATION = 1000;
const unsigned long AUTO_SHOT_DELAY = 500;
const unsigned long RESTART_DELAY = 3000;

const int TARGET_POWER[3] = {2, 3, 4};


// ==================================================
// ETATS DU JEU
// ==================================================

enum GameState {
  WAITING_TARGET,
  WAITING_BUTTON,
  CHARGING,
  GAME_OVER
};

GameState state = WAITING_TARGET;


// ==================================================
// VARIABLES DU JEU
// ==================================================

int target = 0;
int powerLevel = 0;
int score = 0;
int misses = 0;

bool hasMissed = false;

bool buttonState = HIGH;
bool lastButtonReading = HIGH;

bool waitingForVirtualLED = false;

unsigned long lastButtonChangeTime = 0;
unsigned long lastChargeTime = 0;
unsigned long targetStartTime = 0;
unsigned long maxPowerTime = 0;


// ==================================================
// INITIALISATION
// ==================================================

void setup() {
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.begin(115200);

  randomSeed(micros());

  afficherLEDs(0, 0, -1, false);

  Serial.println();
  Serial.println("================================");
  Serial.println("       BIENVENUE AU JEU !");
  Serial.println("================================");

  animationDemarrage();

  jouerFanfare();

  attendreRelachementBouton();

  nouvelleCible();
}


// ==================================================
// BOUCLE PRINCIPALE
// ==================================================

void loop() {
  if (state == GAME_OVER) {
    return;
  }

  unsigned long now = millis();

  lireBouton(now);

  if (state == WAITING_TARGET) {
    if (now - targetStartTime >= TARGET_DELAY) {
      state = WAITING_BUTTON;

      afficherLEDs(0, score, -1, hasMissed);

      Serial.println();
      Serial.println("Cible desactivee.");
      Serial.println("Tu peux maintenant charger !");
    }
  }

  if (state == CHARGING) {
    gererChargement(now);
  }
}


// ==================================================
// ANIMATION DE DEMARRAGE
// ==================================================

void animationDemarrage() {
  Serial.println("Animation de demarrage...");

  for (int i = 1; i <= 5; i++) {
    afficherLEDs(i, 0, -1, false);
    delay(180);
  }

  afficherLEDs(0, 0, -1, false);
  delay(200);

  afficherLEDs(0, 0, -1, true);
  delay(180);

  for (int i = 1; i <= 3; i++) {
    afficherLEDs(0, i, -1, false);
    delay(180);
  }

  for (int i = 0; i < 3; i++) {
    afficherLEDs(0, 0, i, false);
    delay(180);
  }

  afficherLEDs(5, 3, -1, true);
  delay(500);

  afficherLEDs(0, 0, -1, false);
  delay(300);

  Serial.println("Animation terminee !");
}


// ==================================================
// ATTENDRE LE RELACHEMENT DU BOUTON
// ==================================================

void attendreRelachementBouton() {
  Serial.println("Relache le bouton pour commencer.");

  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(10);
  }

  buttonState = HIGH;
  lastButtonReading = HIGH;

  lastButtonChangeTime = millis();
}


// ==================================================
// LECTURE DU BOUTON
// ==================================================

void lireBouton(unsigned long now) {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastButtonChangeTime = now;
    lastButtonReading = reading;
  }

  if (now - lastButtonChangeTime < DEBOUNCE_DELAY) {
    return;
  }

  if (reading == buttonState) {
    return;
  }

  buttonState = reading;

  // Bouton enfonce
  if (buttonState == LOW) {
    if (state == WAITING_BUTTON) {
      commencerChargement();
    }
  }

  // Bouton relache
  else {
    if (state == CHARGING && !waitingForVirtualLED) {
      terminerChargement();
    }
  }
}


// ==================================================
// CREER UNE NOUVELLE CIBLE
// ==================================================

void nouvelleCible() {
  if (score >= 3) {
    terminerPartie(true);
    return;
  }

  if (misses >= 2) {
    terminerPartie(false);
    return;
  }

  powerLevel = 0;
  waitingForVirtualLED = false;

  target = random(0, 3);

  Serial.println();
  Serial.println("-------------------------------");

  Serial.print("Nouvelle cible : ");
  Serial.println(target + 1);

  Serial.print("Puissance requise : ");
  Serial.println(TARGET_POWER[target]);

  afficherLEDs(0, score, target, hasMissed);

  targetStartTime = millis();

  state = WAITING_TARGET;

  Serial.println("La cible apparait pendant 0,5 seconde.");
}


// ==================================================
// COMMENCER LE CHARGEMENT
// ==================================================

void commencerChargement() {
  powerLevel = 0;
  waitingForVirtualLED = false;

  lastChargeTime = millis();

  state = CHARGING;

  afficherLEDs(0, score, -1, hasMissed);

  // Son de depart du chargement
  tone(BUZZER_PIN, 300);

  Serial.println("Chargement commence !");
}


// ==================================================
// GERER LE CHARGEMENT
// ==================================================

void gererChargement(unsigned long now) {

  if (waitingForVirtualLED) {
    if (now - maxPowerTime >= AUTO_SHOT_DELAY) {
      Serial.println("Tir automatique !");
      terminerChargement();
    }

    return;
  }

  if (now - lastChargeTime >= CHARGE_INTERVAL) {
    lastChargeTime = now;

    if (powerLevel < MAX_POWER) {
      powerLevel++;

      Serial.print("Puissance : ");
      Serial.println(powerLevel);

      afficherLEDs(powerLevel, score, -1, hasMissed);

      // Augmenter progressivement la hauteur du son
      int frequence = map(powerLevel, 1, MAX_POWER, 400, 1600);
      tone(BUZZER_PIN, frequence);
    }

    if (powerLevel == MAX_POWER) {
      waitingForVirtualLED = true;
      maxPowerTime = now;

      Serial.println("Puissance maximale atteinte !");
      Serial.println("Tir automatique dans 500 ms...");
    }
  }
}


// ==================================================
// TERMINER LE CHARGEMENT ET EFFECTUER LE TIR
// ==================================================

void terminerChargement() {
  if (state != CHARGING) {
    return;
  }

  state = WAITING_TARGET;
  waitingForVirtualLED = false;

  // Arreter le son de chargement
  noTone(BUZZER_PIN);

  Serial.println();
  Serial.println("========== TIR ==========");

  Serial.print("Puissance utilisee : ");
  Serial.println(powerLevel);

  if (powerLevel == TARGET_POWER[target]) {
    score++;

    Serial.println("TOUCHE ! Point gagne !");

    jouerSonReussite();
  }
  else {
    misses++;
    hasMissed = true;

    Serial.println("RATE !");

    jouerSonEchec();
  }

  afficherLEDs(0, score, -1, hasMissed);

  Serial.print("Score : ");
  Serial.println(score);

  Serial.print("Tirs rates : ");
  Serial.println(misses);

  if (score >= 3) {
    terminerPartie(true);
    return;
  }

  if (misses >= 2) {
    terminerPartie(false);
    return;
  }

  delay(500);

  nouvelleCible();
}


// ==================================================
// AFFICHER LES LED
// ==================================================

void afficherLEDs(
  int puissance,
  int points,
  int cible,
  bool erreur
) {
  byte barre = 0;
  byte indicateurs = 0;

  // Barre de puissance : Q1 a Q5 du premier registre
  for (int i = 1; i <= puissance; i++) {
    barre |= (1 << i);
  }

  // Score : Q1 a Q3 du deuxieme registre
  for (int i = 1; i <= points; i++) {
    indicateurs |= (1 << i);
  }

  // Cibles bleues : Q5 a Q7 du deuxieme registre
  if (cible == 0) {
    indicateurs |= (1 << 5);
  }
  else if (cible == 1) {
    indicateurs |= (1 << 6);
  }
  else if (cible == 2) {
    indicateurs |= (1 << 7);
  }

  // LED rouge persistante
  if (erreur) {
    indicateurs |= (1 << 0);
  }

  digitalWrite(LATCH_PIN, LOW);

  shiftOut(
    DATA_PIN,
    CLOCK_PIN,
    MSBFIRST,
    indicateurs
  );

  shiftOut(
    DATA_PIN,
    CLOCK_PIN,
    MSBFIRST,
    barre
  );

  digitalWrite(LATCH_PIN, HIGH);
}


// ==================================================
// TERMINER ET RECOMMENCER LA PARTIE
// ==================================================

void terminerPartie(bool victoire) {
  state = GAME_OVER;

  noTone(BUZZER_PIN);

  Serial.println();
  Serial.println("================================");

  if (victoire) {
    Serial.println("VICTOIRE ! Tu as gagne !");
  }
  else {
    Serial.println("DEFAITE ! Tu as rate deux tirs.");
  }

  Serial.print("Score final : ");
  Serial.println(score);

  Serial.print("Tirs rates : ");
  Serial.println(misses);

  Serial.println("================================");

  afficherLEDs(0, score, -1, hasMissed || !victoire);

  if (victoire) {
    sonDeVictoire();
  }
  else {
    sonDeDefaite();
  }

  Serial.println("Nouvelle partie dans 3 secondes...");

  delay(RESTART_DELAY);

  score = 0;
  misses = 0;
  powerLevel = 0;

  hasMissed = false;
  waitingForVirtualLED = false;

  afficherLEDs(0, 0, -1, false);

  attendreRelachementBouton();

  Serial.println();
  Serial.println("======= NOUVELLE PARTIE =======");

  nouvelleCible();
}


// ==================================================
// FONCTIONS SONORES
// ==================================================

void jouerNote(int frequence, int duree) {
  tone(BUZZER_PIN, frequence);
  delay(duree);
  noTone(BUZZER_PIN);
  delay(50);
}


// Son de demarrage
void jouerFanfare() {
  jouerNote(600, 150);
  jouerNote(800, 150);
  jouerNote(1000, 250);
}


// Son de reussite d'un tir
void jouerSonReussite() {
  jouerNote(1000, 120);
  jouerNote(1400, 200);
}


// Son d'echec d'un tir
void jouerSonEchec() {
  jouerNote(300, 350);
}


// Son de victoire : melodie ascendante et joyeuse
void sonDeVictoire() {
  jouerNote(523, 150);   // Do
  jouerNote(659, 150);   // Mi
  jouerNote(784, 150);   // Sol
  jouerNote(1047, 350);  // Do aigu
}


// Son de defaite : melodie descendante
void sonDeDefaite() {
  jouerNote(700, 250);
  jouerNote(550, 250);
  jouerNote(400, 350);
  jouerNote(250, 500);
}