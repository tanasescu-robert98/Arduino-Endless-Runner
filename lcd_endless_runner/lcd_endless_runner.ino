#include <NewTone.h>
#include <IRremote.h>
#include <LiquidCrystal_I2C.h>
#include <LedControl.h>

// Life LEDS PINS
#define Lifes_led1 13
#define Lifes_led2 12
#define Lifes_led3 11

// Dot Matrix PINS
#define DIN 10
#define CS 9
#define CLK 8

// IR Sensor PIN
#define RECV_PIN 5

// Joystick Analog Pins
#define Xpin A0
#define Ypin A1

// Buzzer Pin
#define piezoPin 7

// Directional Buttons PINS
#define up_button 4
#define down_button 3
#define attack_button 2

// Variabile globale
int iteratie = 6;
int row = 0;
float SCORE = 0;
int GAME_OVER = -1; // indicator pentru starea jocului , -1 inseamna meniu, 0 inseamna jocul efectiv, 1 inseamna ecranul de GAME OVER
int no_lifes = 3; // numarul de vieti ale player-ului
int button_pressed = 0; 
int Xval; // directia pe axa OX a joystick-ului
int Yval; // directia pe axa OY a joystick-ului
int Sval;
int cnt = 0;
int top[38] = {0};  // vectorul de obstacole pentru cele de pe top
int bot[38] = {0}; // vectorul de obstacole pentru cele de pe bot
int top_iter[38] = {3}; // vector in care retinem cum vor fi grupate obstacolele
int player_pos = 0; // pozitia player-ului, 0 sus , 1 jos
int player_damaged_cursor = 0; // flag care ne spune daca player-ul a fost lovit de obstacol
int counter = 0, to_read;
int j = 1;
int load_iteration = 1;
int iteratie_blink = 1;
int contor_back = 0;

// Dot Matrix Sprites
byte sprite_new[] = {B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000};
byte sprite_over[] = {B11111111,B10000001,B10100101,B10000001,B10111101,B10100101,B10000001,B11111111};
byte sprite_do_it[] = {B01101110,B00101010,B01101110,B00000000,B01011101,B01001001,B01001000,B01001001};

// functie de ridicat la putere
int power_function(int a, int b)
{
  int i = b;
  int c = a;
  while(i > 0)
  {
    c = c * a;
    i--;
  }
  return c;
}

LedControl matrix = LedControl(DIN,CLK,CS,0);
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address 0x27, 16 column and 2 rows

void setup() 
{
  
  // folosesc un pin nefolosit pentru
  // a genera un seed pentru a face
  // obstacolele random
  randomSeed(analogRead(A2));
  
  IrReceiver.begin(RECV_PIN,DISABLE_LED_FEEDBACK);
  
  // Setam pinii de pe arduino
  pinMode(Xpin, INPUT);
  pinMode(Ypin, INPUT);
  pinMode(Lifes_led1, OUTPUT);
  pinMode(Lifes_led2, OUTPUT);
  pinMode(Lifes_led3, OUTPUT);
  // La butoane folosim Input_pullup pentru a activa
  // rezistenta interna din Arduino si a nu risca distrugerea pinilor
  pinMode(up_button, INPUT_PULLUP);
  pinMode(down_button, INPUT_PULLUP);
  pinMode(attack_button, INPUT_PULLUP);
  
  // LCD INITIALIZE
  lcd.init();
  lcd.backlight();

  // Dot Matrix INITIALIZE
  matrix.shutdown(0, false);
  matrix.setIntensity(0, 0);
  matrix.clearDisplay(0);
  
  // Initializam array-ul de obstacole
  for (int i = 16; i < 38; i = i + 6)
  {
      top_iter[i] = random(0, 4);
      if (top_iter[i] == 0)
      {
          top[i] = random(3, 5);
          bot[i+3] = random(3, 5);
      }
      else if (top_iter[i] == 1)
      {
          top[i] = random(3, 5);
          top[i+3] = random(3, 5);
      }
      else if (top_iter[i] == 2)
      {
          bot[i] = random(3, 5);
          bot[i+3] = random(3, 5);
      }
      else if (top_iter[i] == 3)
      {
          bot[i] = random(3, 5);
          top[i+3] = random(3, 5);
      }
  }
}

void loop() {
  
    // verificam starea jocului

    // daca suntem in ecranul de inceput
    if (GAME_OVER == -1)
    {
         // pornim ledurile pe rand
         iteratie_blink++;
         if (iteratie_blink == 2)
         {
            digitalWrite(Lifes_led1,HIGH);
         }
         if (iteratie_blink == 8)
         {
            digitalWrite(Lifes_led2,HIGH);
         }
         if (iteratie_blink == 14)
         {
            digitalWrite(Lifes_led3,HIGH);
         }
         // afisam animatia de loading
         if (load_iteration < 15)
         {
           lcd.setCursor(4, 0);
           lcd.print("LOADING");
           lcd.setCursor(0, 1);
           lcd.print("[");
           lcd.setCursor(15, 1);
           lcd.print("]");
           lcd.setCursor(load_iteration,1);
           lcd.print("#");
           if(load_iteration < 15)
            load_iteration++;
           delay(150);
         }
         
         // afisam press any key to start si scriem pe dot matrix
         // mesajul "DO IT!"
         if (load_iteration == 15 || load_iteration == 16)
         {
             if (load_iteration == 15)
             {
                lcd.clear();
                load_iteration++;
             }
             print_on_matrix(sprite_do_it);
             lcd.setCursor(1, 0);
             lcd.print("PRESS ANY KEY");
             lcd.setCursor(3, 1);
             lcd.print("TO START!");
             // asteptam input de la joystick, butoane sau telecomanda
             int current_state_0 = digitalRead(up_button);
             int current_state_1 = digitalRead(down_button);
             int current_state_2 = digitalRead(attack_button);
             Xval = analogRead(Xpin);
             Yval = analogRead(Ypin);
             if (IrReceiver.decode())
             {
                if (IrReceiver.decodedIRData.decodedRawData == 0xD04CFC60)
                {
                   GAME_OVER = 0;
                }
                else if (IrReceiver.decodedIRData.decodedRawData == 0x61DB14E2)
                {
                   GAME_OVER = 0;
                }
                else if (IrReceiver.decodedIRData.decodedRawData == 0x6F89644)
                {
                   GAME_OVER = 0;
                }
                IrReceiver.resume();
             }
             // daca am primit input pornim jocul
             if (current_state_0 == button_pressed || current_state_1 == button_pressed || current_state_2 == button_pressed || Yval < 40 || Yval > 700 || Xval < 40 || Xval > 700)
                GAME_OVER = 0;
         }
    }
    // incepe jocul
    else if (GAME_OVER == 0)
    {
      
         // in caz de reset anterior al jocului
         // resetam ledurile aprinse de pe
         // dot matrix si numarul de vieti
         if (contor_back == 0)
         {
           sprite_new[0] = B00000000;
           sprite_new[1] = B00000000;
           sprite_new[2] = B00000000;
           sprite_new[3] = B00000000;
           sprite_new[4] = B00000000;
           sprite_new[5] = B00000000;
           sprite_new[6] = B00000000;
           sprite_new[7] = B00000000;
           digitalWrite(Lifes_led1, HIGH);
           digitalWrite(Lifes_led2, HIGH);
           digitalWrite(Lifes_led3, HIGH);
           no_lifes = 3;
           contor_back++;
         }
         
        // decodam semnalele IR
         
         if (IrReceiver.decode())
         {
            // daca apas pe butonul de "in sus" ma duc in sus
            if (IrReceiver.decodedIRData.decodedRawData == 0xD04CFC60)
            {
               player_pos = 0;
            }
            // daca apas pe butonul de "in jos" ma duc in jos
            else if (IrReceiver.decodedIRData.decodedRawData == 0x61DB14E2)
            {
               player_pos = 1;
            }
            // daca apas pe butonul de "in dreapta" am atac
            else if (IrReceiver.decodedIRData.decodedRawData == 0x6F89644)
            {
              // verificam lane-ul de pe care trebuie distrus obstacolul
              if (player_pos == 0)
              {
                if (top[j+1] == 4)
                {
                  top[j+1] = 2;
                  NewTone(piezoPin, 600);
                  delay(300);
                  noNewTone(piezoPin);
                }
              }
              else if (player_pos == 1)
              {
                if (bot[j+1] == 4)
                {
                  bot[j+1] = 2;
                  NewTone(piezoPin, 600);
                  delay(300);
                  noNewTone(piezoPin);
                }
              }
            }
            IrReceiver.resume();
         }
        
        print_on_matrix(sprite_new);
        SCORE = SCORE + 0.01;
        int current_state = digitalRead(attack_button);

        // tratare distrugere obstacol
        
        if (current_state == button_pressed || Xval > 700)
        {
            // verificam pe ce lane se afla player-ul
            // si distrugem obstacolol daca se afla aproape de player
            // si punem un sunet pe buzzer
            if (player_pos == 0)
            {
              if (top[j+1] == 4)
              {
                top[j+1] = 2;
                NewTone(piezoPin, 600);
                delay(300);
                noNewTone(piezoPin);
              }
            }
            else if (player_pos == 1)
            {
              if (bot[j+1] == 4)
              {
                bot[j+1] = 2;
                NewTone(piezoPin, 600);
                delay(300);
                noNewTone(piezoPin);
              }
            }
        }

        // tratare distrugere obstacol end
        
        // citim valorile de pe axele OX si OY de la joystick
        Xval = analogRead(Xpin);
        Yval = analogRead(Ypin);

        // modificare pozitie
        
        if (Yval < 40 || digitalRead(up_button) == 0)
        {
            // mutam player-ul pe top
            player_pos = 0;
        }
        else if (Yval > 700 || digitalRead(down_button) == 0)
        {
            // mutam player-ul pe bot
            player_pos = 1;
        }

        // modificare pozitie end

        // tratare coliziune
        if ( (top[j-1] == 4 && player_pos == 0) || (top[j-1] == 3 && player_pos == 0) || (bot[j-1] == 3 && player_pos == 1) || (bot[j-1] == 4 && player_pos == 1) )
        {
            // in caz de coliziune
            // avem un sunet facut de buzzer
            // jucatorul pierde o viata
            // si daca era la ultima viata pierde jocul
            player_damaged_cursor = 1;
            NewTone(piezoPin, 300);
            delay(400);
            noNewTone(piezoPin); // Turn off the tone.
            no_lifes--;
            if (no_lifes == 2)
            {
              digitalWrite(Lifes_led3, LOW);
            }
            else if (no_lifes == 1)
            {
              digitalWrite(Lifes_led2, LOW);
            }
            else if (no_lifes == 0)
            {
              digitalWrite(Lifes_led1, LOW);
              GAME_OVER = 1;
            }
        }
        // tratare coliziune end

        // reinitializare array dupa ce s-a ajuns la sfarsit
        // si generam obstacole noi
        
        if (j == 39)
        {
            for (int i = 0; i < 38; i++)
            {
                top[i] = 0;
                bot[i] = 0;
            }
            for (int i = 16; i < 38; i = i + 6)
            {
                top_iter[i] = random(0, 4);
                if (top_iter[i] == 0)
                {
                    top[i] = random(3, 5);
                    bot[i+3] = random(3, 5);
                }
                else if (top_iter[i] == 1)
                {
                    top[i] = random(3, 5);
                    top[i+3] = random(3, 5);
                }
                else if (top_iter[i] == 2)
                {
                    bot[i] = random(3, 5);
                    bot[i+3] = random(3, 5);
                }
                else if (top_iter[i] == 3)
                {
                    bot[i] = random(3, 5);
                    top[i+3] = random(3, 5);
                }
            }
            j = 1;
        }

        // reinitializare array dupa ce s-a ajuns la sfarsit end
        
        // facem refresh la LCD
        lcd.clear();
        
        //Draw Player
        
        // daca nu a fost lovit
        if (player_damaged_cursor == 0)
        {
            lcd.setCursor(0, player_pos);
            lcd.print(")");
        }
        // daca a fost lovit de un obstacol desenam un "X" in locul
        // player-ului pentru putin timp
        else if (player_damaged_cursor != 0)
        {
          lcd.setCursor(0, player_pos);
          lcd.print("X");
          player_damaged_cursor++;
          if (player_damaged_cursor == 8)
          {
              player_damaged_cursor = 0;
          }
        }
        // Draw Player End

        // Draw Obstacles
        // desenam obstacolele in functie
        // de lane si tipul lor
        for (int i = 0; i < 38; i++)
        {
            if (top[i] == 3)
            {
                if (i - j > -1)
                {
                  lcd.setCursor(i-j, 0);
                  lcd.print("["); 
                }
            }
            if (bot[i] == 3)
            {
                if (i - j > -1)
                {
                  lcd.setCursor(i-j, 1);
                  lcd.print("["); 
                }
            }
            if (top[i] == 4)
            {
                if(i - j > -1)
                {
                  lcd.setCursor(i-j, 0);
                  lcd.print("#"); 
                }
            }
            if (bot[i] == 4)
            {
                if (i - j > -1)
                {
                  lcd.setCursor(i-j, 1);
                  lcd.print("#"); 
                }
            }

            // Draw Obstacles end

            // Draw Damaged Obstacles
            // obstacolele distruse le desenez
            // ca fiind ":" pentru putin timp pentru
            // a crea un fel de animatie
            // dupa care dispar
            // si daca s-a umplut o linie pe dot-matrix
            // player-ul primeste fie o viata in plus
            // sau bonus la scor
            if (bot[i] == 2 && i - j != 0)
            {
                  if (i - j > -1)
                  {
                      lcd.setCursor(i-j, 1);
                      lcd.print(":");
                      if (iteratie >= -1)
                      {
                          if (sprite_new[row] == 254)
                          {
                            sprite_new[row] = sprite_new[row] + 1;
                            iteratie--;
                            row++;
                            iteratie = 6;
                            if (no_lifes == 3)
                            {
                              SCORE = SCORE + 1.5f;
                            }
                            else if (no_lifes == 2)
                            {
                              no_lifes = 3;
                              digitalWrite(Lifes_led3, HIGH);
                            }
                            else if (no_lifes == 1)
                            {
                              no_lifes = 2;
                              digitalWrite(Lifes_led2, HIGH);
                            }
                          }
                          else if (sprite_new[row] < 254)
                          {
                            sprite_new[row] = sprite_new[row] + power_function(2,iteratie);
                            iteratie--;
                          }
                       } 
                   }
            }
            if (top[i] == 2 && i - j != 0)
            {
                  if (i - j > -1)
                  {
                      lcd.setCursor(i-j, 0);
                      lcd.print(":"); 
                      if (iteratie >= -1)
                      {
                          if (sprite_new[row] == 254)
                          {
                            sprite_new[row] = sprite_new[row] + 1;
                            iteratie--;
                            row++;
                            iteratie = 6;
                            if (no_lifes == 3)
                            {
                              SCORE = SCORE + 3;
                            }
                            else if (no_lifes == 2)
                            {
                              no_lifes = 3;
                              digitalWrite(Lifes_led3, HIGH);
                            }
                            else if (no_lifes == 1)
                            {
                              no_lifes = 2;
                              digitalWrite(Lifes_led2, HIGH);
                            }
                          }
                          else if (sprite_new[row] < 254)
                          {
                            sprite_new[row] = sprite_new[row] + power_function(2,iteratie);
                            iteratie--;
                          }
                      }
                   }
            }

            // Draw Damaged Obstacles end
            
        }
        j++;
        delay(300);
    }
    // daca suntem in ecranul de Game Over
    else if (GAME_OVER == 1)
    {
        // afisam fata suparata pe matrice si scriem pe lcd
        if (cnt == 0)
        {
            print_on_matrix(sprite_over);
            lcd.clear();
            lcd.setCursor(3, 0);
            lcd.print("GAME OVER!");
            lcd.setCursor(0, 1);
            lcd.print("SCORE:");
            lcd.setCursor(6, 1);
            lcd.print(SCORE);
            cnt++;
        }
        // verificam daca avem input de la butoane sau joystick
        int current_state_0 = digitalRead(up_button);
        int current_state_1 = digitalRead(down_button);
        int current_state_2 = digitalRead(attack_button);
        Xval = analogRead(Xpin);
        Yval = analogRead(Ypin);
        
        // daca se apasa un buton sau se misca joystick-ul, jocul se restarteaza
        if (current_state_0 == button_pressed || current_state_1 == button_pressed || current_state_2 == button_pressed || Yval < 40 || Yval > 700 || Xval < 40 || Xval > 700)
        {
            // resetam toate variabilele globale si vectorii de obstacole si trecem inapoi la joc
            GAME_OVER = 0;
            contor_back = 0;
            iteratie = 6;
            row = 0;
            SCORE = 0;
            no_lifes = 3;
            button_pressed = 0;
            cnt = 0;
            j = 1;
            load_iteration = 1;
            iteratie_blink = 1;
            contor_back = 0;
            for (int i = 0 ; i < 38; i++)
            {
              top[i] = 0;
              bot[i] = 0;
              top_iter[i] = 3;
            }
            matrix.clearDisplay(0);
            player_damaged_cursor = 0;
            for (int i = 16; i < 38; i = i + 6)
            {
                top_iter[i] = random(0, 4);
                if (top_iter[i] == 0)
                {
                    top[i] = random(3, 5);
                    bot[i+3] = random(3, 5);
                }
                else if (top_iter[i] == 1)
                {
                    top[i] = random(3, 5);
                    top[i+3] = random(3, 5);
                }
                else if (top_iter[i] == 2)
                {
                    bot[i] = random(3, 5);
                    bot[i+3] = random(3, 5);
                }
                else if (top_iter[i] == 3)
                {
                    bot[i] = random(3, 5);
                    top[i+3] = random(3, 5);
                }
            }
        }
    }
}

// functie pentru printarea pe matrice a unui "sprite"
void print_on_matrix(byte character [])
{
  int i = 0;
  for (i = 0; i < 8; i++)
  {
    matrix.setRow(0, i, character[i]);
  }
}
 
