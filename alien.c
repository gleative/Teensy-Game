#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <math.h> // For unsing of round

#include <stdlib.h> // For rand function

#include "cpu_speed.h"
#include "graphics.h"
#include "lcd.h"
#include "sprite.h"

#include "usb_serial.h"

#define FREQUENCY 8000000.0
#define PRESCALER 1024.0

#define SPACECRAFT_HEIGHT (5)
#define SPACECRAFT_WIDTH (8)

#define ALIEN_HEIGHT (5)
#define ALIEN_WIDTH (6)

#define MOTHERSHIP_HEIGHT (10)
#define MOTHERSHIP_WIDTH (16)

#define BULLET_SPEED (1.00)

// Function declarations
void debounce_PINF(int pin_number);
void start_count_down();
void draw_border();
void draw_status();
void draw_game();
void game_over_screen();
void restart_game();
int sprite_x_location(Sprite *sprite);
int sprite_y_location(Sprite *sprite);
void sprite_move(Sprite *sprite, int dx, int dy);
void sprite_turn_to(Sprite *sprite, double dx, double dy);
void sprite_step(Sprite *sprite);
void attack_spaceCraft(Sprite *alien, Sprite *spaceCraft, int alien_type);
void make_sprite_invisible(Sprite *sprite, int value);
void collision_bullet_alien(Sprite *bullet, Sprite *alien, int s_craft);
int random_number();
void send_debug_string(char* string);
double get_system_time(unsigned int timer_count);
double get_system_time_debugger();
void write_spacecraft_info_to_therminal();
void move_sprite_away_from_playfield(Sprite *sprite);
void sprite_new_position(Sprite *sprite, int x, int y);
void check_mothership_lives(Sprite *m_ship);
void mothership_shoot_bullet(Sprite *m_bullet);
void update_healthbar(Sprite *m_ship);

// Global variables
// True = 1, False = 0
int game_over = 0;
// 0=up, 1=down, 2=left, 3=right
int spaceCraft_direction = 0;
double curr_time = 0.0;
int seed = 0;
int bullet_on_screen = 0;
int m_bullet_on_screen = 0; // Mothership's bullet
int attack = 0;
int is_in_corner = 0;

// For random numbers
int second_counter = 0;
int rn = 0;
int rn2 = 0; // Number for when mothership shoots
int second_counter2 = 0;

int timer1_overflow_counter = 0; // Used for get_system_time_debugger
int timer0_overflow_counter = 0; // Used for counting each 0.5 seconds
int boss_battle = 0; //0 if its alien, 1 if its mothership
int alien_killed = 0; // Holds the value amount of time alien is killed
int mothership_lives = 10;

// Intro menu variables START
char * gameTitle = "Alien Advanced";
char * firstName = "Glenn";
char * lastName = "Christensen";
char * stdNummer = "n9884050";
char * pressBtnMsg = "Press button to";
char * pressBtnMsg2 = "play";

// Times 5 because char witdth is 5
int gameTitleLength = 14*5;
int firstNameLength = 5*5;
int lastNameLength = 11*5;
int stdNumLength = 8*5;
int pressBtnLength = 15*5;
int pressBtnLength2 = 4*5;

// Intro Menu variables END

// Game over menu START
char * gameOverMsg = "GAME OVER";
char * gameOverMsg2 = "Your dead!";
char * gameOverMsg3 = "Press a button";
char * gameOverMsg4 = "to play again";

int gameOverMsgLength = 9*5;
int gameOverMsgLength2 = 10*5;
int gameOverMsgLength3 = 14*5;
int gameOverMsgLength4 = 13*5;
// Game over menu END

// Coordinates START
int x_center = LCD_X/2;
int y_center = LCD_Y/2;

int x_left = 0;
int x_right = (LCD_X-1);
int y_bottom = (LCD_Y-1);
int y_top = 0;
// Coordinates END

// Player stats START
int lives = 11; // For some reason i get -1 health when i start the game. This is why i have it to 11 lives. When restart game, it doesnt get deicremented
int score = 0;
int status_timer = 0;
int seconds = 0;
int sec_seconds = 0;
int minutes = 0;
int min_minutes = 0;
// Player stats END

// Sprites
Sprite spaceCraft;
Sprite alien;
Sprite bullet;
Sprite mothership;
Sprite mothership_bullet;

unsigned char spaceCraft_sprite_up[] = {
  0b00011000,
  0b00111100,
  0b01111110,
  0b11111111,
  0b11100111,
};

unsigned char spaceCraft_sprite_down[] = {
  0b11100111,
  0b11111111,
  0b01111110,
  0b00111100,
  0b00011000,
};

unsigned char spaceCraft_sprite_left[] = {
  0b00001111,
  0b01111111,
  0b11111110,
  0b01111111,
  0b00001111,
};

unsigned char spaceCraft_sprite_right[] = {
  0b11110000,
  0b11111110,
  0b01111111,
  0b11111110,
  0b11110000,
};

unsigned char alienCraft_sprite[] = {
  0b10000100,
  0b11111100,
  0b11001100,
  0b11111100,
  0b10000100,
};

unsigned char bullet_sprite[] = {
  0b11000000,
  0b11000000,
};

unsigned char bullet_sprite_mothership[] = {
  0b11000000,
  0b11000000,
};

unsigned char mothership_sprite_10_health[] = {
  0b11111111,0b11111111,
  0b00000000,0b00000000,
  0b00011111,0b11111000,
  0b00111111,0b11111100,
  0b01100001,0b10000110,
  0b11110011,0b11001111,
  0b11111111,0b11111111,
  0b11111110,0b01111111,
  0b01111110,0b01111110,
  0b00111111,0b11111100,

};

unsigned char mothership_sprite_9_health[] = {
  0b11111111,0b11111100,
  0b00000000,0b00000000,
  0b00011111,0b11111000,
  0b00111111,0b11111100,
  0b01100001,0b10000110,
  0b11110011,0b11001111,
  0b11111111,0b11111111,
  0b11111110,0b01111111,
  0b01111110,0b01111110,
  0b00111111,0b11111100,
};

unsigned char mothership_sprite_8_health[] = {
  0b11111111,0b11110000,
  0b00000000,0b00000000,
  0b00011111,0b11111000,
  0b00111111,0b11111100,
  0b01100001,0b10000110,
  0b11110011,0b11001111,
  0b11111111,0b11111111,
  0b11111110,0b01111111,
  0b01111110,0b01111110,
  0b00111111,0b11111100,
};

unsigned char mothership_sprite_7_health[] = {
  0b11111111,0b11100000,
  0b00000000,0b00000000,
  0b00011111,0b11111000,
  0b00111111,0b11111100,
  0b01100001,0b10000110,
  0b11110011,0b11001111,
  0b11111111,0b11111111,
  0b11111110,0b01111111,
  0b01111110,0b01111110,
  0b00111111,0b11111100,
};

unsigned char mothership_sprite_6_health[] = {
  0b11111111,0b11000000,
  0b00000000,0b00000000,
  0b00011111,0b11111000,
  0b00111111,0b11111100,
  0b01100001,0b10000110,
  0b11100001,0b10000111,
  0b11111111,0b11111111,
  0b11110000,0b00001111,
  0b01100000,0b00000110,
  0b00111111,0b11111100,
};

unsigned char mothership_sprite_5_health[] = {
  0b11111111,0b00000000,
  0b00000000,0b00000000,
  0b00011111,0b11111000,
  0b00111111,0b11111100,
  0b01100001,0b10000110,
  0b11100001,0b10000111,
  0b11111111,0b11111111,
  0b11110000,0b00001111,
  0b01100000,0b00000110,
  0b00111111,0b11111100,
};

unsigned char mothership_sprite_4_health[] = {
  0b11111100,0b00000000,
  0b00000000,0b00000000,
  0b00011111,0b11111000,
  0b00111111,0b11111100,
  0b01100001,0b10000110,
  0b11100001,0b10000111,
  0b11111111,0b11111111,
  0b11110000,0b00001111,
  0b01100000,0b00000110,
  0b00111111,0b11111100,
};

unsigned char mothership_sprite_3_health[] = {
  0b11110000,0b00000000,
  0b00000000,0b00000000,
  0b00011111,0b11111000,
  0b00111111,0b11111100,
  0b01110001,0b10001110,
  0b11100001,0b10000111,
  0b11111111,0b11111111,
  0b11110000,0b00001111,
  0b01100000,0b00000110,
  0b00111111,0b11111100,
};

unsigned char mothership_sprite_2_health[] = {
  0b11100000,0b00000000,
  0b00000000,0b00000000,
  0b00011111,0b11111000,
  0b00111111,0b10110100,
  0b01110001,0b10001110,
  0b11100001,0b10000111,
  0b10111111,0b10011111,
  0b10110000,0b00001111,
  0b01100000,0b00000110,
  0b00111111,0b11111100,
};

unsigned char mothership_sprite_1_health[] = {
  0b10000000,0b00000000,
  0b00000000,0b00000000,
  0b00011111,0b11111000,
  0b00110011,0b10101100,
  0b01110001,0b10001110,
  0b11100001,0b10000111,
  0b10100111,0b10011111,
  0b10110000,0b00001111,
  0b01100000,0b00000110,
  0b00111111,0b11111100,
};

void setup_ports(){
  set_clock_speed(CPU_8MHz);

  // LEDs
  DDRB |= 0b1<<2; //LED0, B2
  DDRB |= 0b1<<3; //LED1, B3

  // Buttons
  DDRF &= ~(0b1<<5); //SW2
  DDRF &= ~(0b1<<6); //SW1

  // Joystick
  DDRB &= ~(0b1<<0); //B0 center
  DDRB &= ~(0b1<<7); //B7 down
  DDRB &= ~(0b1<<1); //B1 left
  DDRD &= ~(0b1<<1); //D1 up
  DDRD &= ~(0b1<<0); //D0 right

  // Timers
  // Set timer to normal mode
  TCCR1A = 0x00;

  // Set prescaler to 1024
  TCCR1B |= (1<<CS10);
  TCCR1B &= ~(1<<CS11);
  TCCR1B |= (1<<CS12);

  // Setup timer0 for compare interrupt
  TCCR0A = (1 << WGM01);
  // Set the prescaler on timer0 to 1024
  TCCR0B = (1 << CS02) | (1 << CS00);

  OCR0A = 117;
  // Enable compare interupt on timer0
  TIMSK0 = (1 << OCIE0A);

  OCR1A = 15625;

  // Enable interupt on timer1
  TIMSK1 |= 1<<TOIE1;

  //Enable global interupts
  sei();

  // Making LCD ready to receive
  DDRC |= 0b1<<7;
  // Turn on the pin
  PORTC |= 0b1<<7;

  // Initialise USB serial
  usb_init();

  // Initialize LCD screen
  lcd_init(LCD_DEFAULT_CONTRAST);
  clear_screen();
}

double get_system_time(unsigned int timer_count){
  return timer_count*PRESCALER/FREQUENCY;
}

double get_system_time_debugger(){
  return (timer1_overflow_counter * 65536 + TCNT1) * PRESCALER / FREQUENCY;
}

// This functions is called upon in count_down()
void init_sprites(){
  srand(seed);
  // Unsigned int so it never becomes negative number. Then we dont have to think that X value will be -1 and will go out of border in X axis
  // But this works only for X axis, not the Y, because it cant be less than 9
  // 84 - 8 = 76, have to be 75 is max X, min is 0
  unsigned int spaceCraftX = rand() % LCD_X - SPACECRAFT_WIDTH;
  // 48 - 5 = 43, have to be 42 is max Y, min is 9
  int spaceCraftY = rand() % (LCD_Y - SPACECRAFT_HEIGHT);

  // 84 - 8 = 76, have to be 75 is max X, min is 0
  unsigned int alienX = rand() % LCD_X - ALIEN_WIDTH;
  // 48 - 5 = 43, have to be 42 is max Y, min is 9
  int alienY = rand() % LCD_Y - ALIEN_HEIGHT;

  // If randomX value is bigger than 75, make it 75. Which is max for being inside border
  if(spaceCraftX > 75){
    spaceCraftX = 75;
  }

  if(spaceCraftY > 42){
    spaceCraftY = 42;
  }
  else if(spaceCraftY < 9){
    spaceCraftY = 9;
  }

  if(alienX > 74){
    alienX = 74;
  }
  else if(alienX < 1){
    alienX = 1;
  }

  if(alienY > 42){
    alienY = 42;
  }
  else if(alienY < 10){
    alienY = 10;
  }

  init_sprite(&spaceCraft, spaceCraftX , spaceCraftY, SPACECRAFT_WIDTH, SPACECRAFT_HEIGHT, spaceCraft_sprite_up);
  init_sprite(&alien, alienX, alienY, ALIEN_WIDTH, ALIEN_HEIGHT, alienCraft_sprite);
  init_sprite(&bullet, 0,0, 2, 2, bullet_sprite);
  init_sprite(&mothership, -20,-20, MOTHERSHIP_WIDTH, MOTHERSHIP_HEIGHT, mothership_sprite_10_health);
  init_sprite(&mothership_bullet, -20, -20, 2, 2, bullet_sprite_mothership);
  // Make bullet invisible straight away
  make_sprite_invisible(&bullet, 0);
  make_sprite_invisible(&mothership_bullet, 0);

  seed += 10; // When its game over and restart game, the alien will spawn in new location after being hit once
}

void init_spaceCraft(){
  srand(seed);

  unsigned int spaceCraftX = rand() % LCD_X - SPACECRAFT_WIDTH;
  int spaceCraftY = rand() % (LCD_Y - SPACECRAFT_HEIGHT);

  // If randomX value is bigger than 75, make it 75. Which is max for being inside border
  if(spaceCraftX > 75){
    spaceCraftX = 75;
  }

  if(spaceCraftY > 42){
    spaceCraftY = 42;
  }
  else if(spaceCraftY < 9){
    spaceCraftY = 9;
  }

  sprite_new_position(&spaceCraft, spaceCraftX, spaceCraftY);

}

void init_alien(){
  int spaceCraft_top = round(sprite_y_location(&spaceCraft));
  int spaceCraft_bottom = SPACECRAFT_HEIGHT - 1 + spaceCraft_top;
  int spaceCraft_left = round(sprite_x_location(&spaceCraft));
  int spaceCraft_right = SPACECRAFT_WIDTH - 1 + spaceCraft_left;

  srand(seed);

  // 84 - 8 = 76, have to be 75 is max X, min is 1
  unsigned int alienX = rand() % LCD_X - ALIEN_WIDTH;
  // 48 - 5 = 43, have to be 42 is max Y, min is 9
  int alienY = rand() % LCD_Y - ALIEN_HEIGHT;

  if(alienX > 74){
    alienX = 74;
  }
  else if(alienX < 2){
    alienX = 2;
  }

  if(alienY > 41){
    alienY = 41;
  }
  else if(alienY < 10){
    alienY = 10;
  }

  //If the alien is spawned between the spaceCraft get new values
  if(alienX > spaceCraft_left && alienX < spaceCraft_right || alienY > spaceCraft_top && alienY < spaceCraft_bottom){
    alienX = rand() % LCD_X - ALIEN_WIDTH;
    alienY = rand() % LCD_Y - ALIEN_HEIGHT;

    if(alienX > 74){
      alienX = 74;
    }
    else if(alienX < 2){
      alienX = 2;
    }

    if(alienY > 41){
      alienY = 41;
    }
    else if(alienY < 10){
      alienY = 10;
    }
  }

  sprite_new_position(&alien, alienX, alienY);
}

void init_mothership(){
  int spaceCraft_top = round(sprite_y_location(&spaceCraft));
  int spaceCraft_bottom = SPACECRAFT_HEIGHT - 1 + spaceCraft_top;
  int spaceCraft_left = round(sprite_x_location(&spaceCraft));
  int spaceCraft_right = SPACECRAFT_WIDTH - 1 + spaceCraft_left;

  // 84 - 16 = 68, max X = 67, min = 1
  unsigned int mothershipX = rand() % LCD_X - MOTHERSHIP_WIDTH;
  // 48 - 10 = 38, max Y = 37, min is 9
  int mothershipY = rand() % LCD_Y - MOTHERSHIP_HEIGHT;

  if(mothershipX > 66){
    mothershipX = 66;
  }
  else if(mothershipX < 1){
    mothershipX = 1;
  }
  if(mothershipY > 36){
    mothershipY = 36;
  }
  else if(mothershipY < 10){
    mothershipY = 10;
  }

  //If the mothership is spawned between the spaceCraft get new values
  if((mothershipX > spaceCraft_left && mothershipX < spaceCraft_right) || (mothershipY > spaceCraft_top && mothershipY < spaceCraft_bottom)){
    mothershipX = rand() % LCD_X - ALIEN_WIDTH;
    mothershipY = rand() % LCD_Y - ALIEN_HEIGHT;

    if(mothershipX > 66){
      mothershipX = 66;
    }
    else if(mothershipX < 1){
      mothershipX = 1;
    }

    if(mothershipY > 36){
      mothershipY = 36;
    }
    else if(mothershipY < 10){
      mothershipY = 10;
    }
  }

  // Spawns it in a new location
  sprite_new_position(&mothership, mothershipX, mothershipY);

}

// Moves the sprite, dependent on direction
void move_spaceCraft(int16_t curr_char){
  // Holds the char that the user pressed on the keyboard
  // Spacecraft location
  int sx = sprite_x_location(&spaceCraft);
  int sy = sprite_y_location(&spaceCraft);

  // If joystick is up.
  if(((PIND>>1) & 0b1 && sy > 9) || (curr_char == 'w' && sy > 9)){ //Bigger than 9, because of the status bar at top.
    spaceCraft_direction = 0;
    sprite_move(&spaceCraft, 0, -1);
  }

  // If joystick is down.
  if(((PINB>>7) & 0b1 && sy + SPACECRAFT_HEIGHT < y_bottom) || (curr_char == 's' && sy + SPACECRAFT_HEIGHT < y_bottom)){
    spaceCraft_direction = 1;
    sprite_move(&spaceCraft, 0, 1);
  }

  // If joystick is left.
  if((PINB>>1) & 0b1 && sx > 1 || (curr_char == 'a' && sx > 0)){
    spaceCraft_direction = 2;
    sprite_move(&spaceCraft, -1, 0);
  }

  // If joystick is right.
  if(((PIND>>0) & 0b1 && sx + SPACECRAFT_WIDTH < x_right) || (curr_char == 'd' && sx + SPACECRAFT_WIDTH < x_right)){
    spaceCraft_direction = 3;
    sprite_move(&spaceCraft, +1, 0);
  }
}

void move_alien(Sprite *alien){

  if(boss_battle == 0){
    sprite_step(alien);

    int alien_top = round(sprite_y_location(alien));
    int alien_bottom = ALIEN_HEIGHT - 1 + alien_top;
    int alien_left = round(sprite_x_location(alien));
    int alien_right = ALIEN_WIDTH - 1 + alien_left;

    // If its not within the border
    if(!(alien_top > 10 && alien_bottom < LCD_Y - 2 && alien_left > 1 && alien_right < LCD_X - 2)){
      attack_spaceCraft(alien, &spaceCraft, 0); // Update direction of spacecraft
      attack = 0; // Stop sttacking
      rn = random_number(); // Get new value for when to do the next dash attack
    }

  }
  else if(boss_battle == 1){
    sprite_step(alien);

    int mothership_top = round(sprite_y_location(alien));
    int mothership_bottom = MOTHERSHIP_HEIGHT - 1 + mothership_top;
    int mothership_left = round(sprite_x_location(alien));
    int mothership_right = MOTHERSHIP_WIDTH - 1 + mothership_left;

    // If its not within the border
    if(!(mothership_top > 10 && mothership_bottom < LCD_Y - 1 && mothership_left > 1 && mothership_right < LCD_X - 2)){
      attack_spaceCraft(alien, &spaceCraft, 0);
      attack = 0;
      rn = random_number();
    }

  }


  // Reset the counter so it will count to next time it should attack
  second_counter = 0;

}

void shoot_bullet(Sprite *bullet){
  // Make it visible
  make_sprite_invisible(bullet, 1);

  // If bullet is not on screen (false) you can fire bullet
  if(bullet_on_screen == 0){
    // Tell program bullet is on screen (true) so it cant fire a new bullet
    bullet_on_screen = 1;
    // If spacecraft is up when shooting
    if(spaceCraft_direction == 0){
      // Pluss it with half of the width of ship
      bullet->x = sprite_x_location(&spaceCraft) + 3;
      bullet->y = sprite_y_location(&spaceCraft);

      // Shoots upwards, Here you controll speed of bullet aswell
      sprite_turn_to(bullet, 0, -BULLET_SPEED);
    }
    // If spacecraft is down when shooting
    else if(spaceCraft_direction == 1){
      bullet->x = sprite_x_location(&spaceCraft) + 3;
      bullet->y = sprite_y_location(&spaceCraft);

      sprite_turn_to(bullet, 0, BULLET_SPEED);
    }
    // If spacecraft is left
    else if(spaceCraft_direction == 2){
      bullet->x = sprite_x_location(&spaceCraft);
      bullet->y = sprite_y_location(&spaceCraft) + 1;

      sprite_turn_to(bullet, -BULLET_SPEED, 0);
    }
    // If spacecraft is right
    else if(spaceCraft_direction == 3){
      bullet->x = sprite_x_location(&spaceCraft);
      bullet->y = sprite_y_location(&spaceCraft) + 1;

      sprite_turn_to(bullet, BULLET_SPEED, 0);
    }
  }
}

void update_bullet(Sprite *bullet, int bullet_type){
  // If the bullet is visible, or shot
  if(bullet->is_visible == 1){
    sprite_step(bullet);
  }

  // If its the spacecraft's bullet
  if(bullet_type == 0){
    // If bullet hit one of the borders, hdie it, and tell program bullet is not on screen
    if(sprite_y_location(bullet) < 9 || sprite_y_location(bullet) > LCD_Y || sprite_x_location(bullet) < 0 || sprite_x_location(bullet) > LCD_X){
      // Make it invisible
      make_sprite_invisible(bullet, 0);
      bullet->x = 0;
      bullet->y = 0;
      bullet_on_screen = 0;
    }
  }
  // If its the mothership's bullet
  else if(bullet_type == 1){
    // Se last comparison at back, if mothership is dead, the bullet goes away aswell
    if(sprite_y_location(bullet) < 9 || sprite_y_location(bullet) > LCD_Y || sprite_x_location(bullet) < 0 || sprite_x_location(bullet) > LCD_X || mothership_lives == 0){
      // Make it invisible
      make_sprite_invisible(bullet, 0);
      bullet->x = 0;
      bullet->y = 0;
      m_bullet_on_screen = 0;
    }
  }
}

void mothership_shoot_bullet(Sprite *m_bullet){
  // If no bullet from mothership is on the screen, you can shoot
  if(m_bullet_on_screen == 0){
    m_bullet->x = sprite_x_location(&mothership);
    m_bullet->y = sprite_y_location(&mothership);

    m_bullet_on_screen = 1; // Now not able to shoot a new one, until its 0 again
    make_sprite_invisible(m_bullet, 1);
    attack_spaceCraft(m_bullet, &spaceCraft, 2); // 2 represents that its a bullet going towards spaceCraft
  }
}

void update_spaceCraft_direction(Sprite *spaceCraft, int direction){
  if(direction == 0){
    spaceCraft->bitmap = spaceCraft_sprite_up;
  }
  else if(direction == 1){
    spaceCraft->bitmap = spaceCraft_sprite_down;
  }
  else if(direction == 2){
    spaceCraft->bitmap = spaceCraft_sprite_left;
  }
  else if(direction == 3){
    spaceCraft->bitmap = spaceCraft_sprite_right;
  }
}

void check_mothership_lives(Sprite *m_ship){
  // If mothership has no lives left, change boss battle, remove mothership, and spawn alien
  if(mothership_lives == 0){
    boss_battle = 0;
    alien_killed = 0;
    attack = 0; // So alien that spawns dont instantly attack
    mothership_lives = 10; // Reset lives so it wont loop more than once
    score += 10; // Player gets 10 points for killing mothership.
    init_alien();
    sprite_new_position(&mothership, -20, -20); // Hide mothership from playfield
    m_ship->bitmap = mothership_sprite_10_health; // Reset the map again so it has full health bar when spawned again
    seed += 10; // Neeed to increase this, or alien wont spawn new place when it right after mothership died
    rn = random_number();
    second_counter = 0;
    send_debug_string("Player destroyed mothership");
  }
}

void update_healthbar(Sprite *m_ship){
  if(mothership_lives == 9){
    m_ship->bitmap = mothership_sprite_9_health;
  }
  else if(mothership_lives == 8){
    m_ship->bitmap = mothership_sprite_8_health;
  }
  else if(mothership_lives == 7){
    m_ship->bitmap = mothership_sprite_7_health;
  }
  else if(mothership_lives == 6){
    m_ship->bitmap = mothership_sprite_6_health;
  }
  else if(mothership_lives == 5){
    m_ship->bitmap = mothership_sprite_5_health;
  }
  else if(mothership_lives == 4){
    m_ship->bitmap = mothership_sprite_4_health;
  }
  else if(mothership_lives == 3){
    m_ship->bitmap = mothership_sprite_3_health;
  }
  else if(mothership_lives == 2){
    m_ship->bitmap = mothership_sprite_2_health;
  }
  else if(mothership_lives == 1){
    m_ship->bitmap = mothership_sprite_1_health;
  }
}

void attack_spaceCraft(Sprite *alien, Sprite *spaceCraft, int alien_type){
  double sx = spaceCraft->x;
  double sy = spaceCraft->y;

  double ax = alien->x;
  double ay = alien->y;

  // Spacecraft x position minus alien x position. Abs because so we get absolute value
  double dx = sx - ax;
  double dy = sy - ay;

  double dist = sqrt(dx * dx + dy * dy);

  // If 0 its normal alien
  if(alien_type == 0){
    // 0.90 is the one that controls velocity. The higher number, the faster
    dx = dx * 0.8 / dist;
    dy = dy * 0.8 / dist;
  }
  // If 1 its mothership, it moves half the speed of normal alien
  else if(alien_type == 1){
    // 0.90 is the one that controls velocity. The higher number, the faster
    dy = dy * 0.4 / dist;
    dx = dx * 0.4 / dist;
  }
  // If its the bullet from mothership
  else if(alien_type == 2){
    // 0.90 is the one that controls velocity. The higher number, the faster
    dy = dy * BULLET_SPEED / dist;
    dx = dx * BULLET_SPEED / dist;
  }

  sprite_turn_to(alien, dx, dy);

}

void collision(Sprite *spaceCraft, Sprite *alien){
  int spaceCraft_top = round(sprite_y_location(spaceCraft));
  int spaceCraft_bottom = SPACECRAFT_HEIGHT - 1 + spaceCraft_top;
  int spaceCraft_left = round(sprite_x_location(spaceCraft));
  int spaceCraft_right = SPACECRAFT_WIDTH - 1 + spaceCraft_left;

  if(boss_battle == 0){
    int alien_top = round(sprite_y_location(alien));
    int alien_bottom = ALIEN_HEIGHT - 1 + alien_top;
    int alien_left = round(sprite_x_location(alien));
    int alien_right = ALIEN_WIDTH - 1 + alien_left;

    if(!(spaceCraft_top > alien_bottom || alien_top > spaceCraft_bottom || spaceCraft_left > alien_right || alien_left > spaceCraft_right)){
      send_debug_string("Player killed by alien");
      init_spaceCraft();
      lives--;
      seed += 10;
    }
  }
  else if(boss_battle == 1){
    int mothership_top = round(sprite_y_location(alien));
    int mothership_bottom = MOTHERSHIP_HEIGHT - 1 + mothership_top;
    int mothership_left = round(sprite_x_location(alien));
    int mothership_right = MOTHERSHIP_WIDTH - 1 + mothership_left;

    if(!(spaceCraft_top > mothership_bottom || mothership_top > spaceCraft_bottom || spaceCraft_left > mothership_right || mothership_left > spaceCraft_right)){
      send_debug_string("Mothership destroyed player");
      init_spaceCraft();
      lives--;
      seed += 10;
    }
  }

}

void collision_bullet_alien(Sprite *bullet, Sprite *alien, int s_craft){
  int bullet_top = round(sprite_y_location(bullet));
  int bullet_bottom = 2 - 1 + bullet_top;
  int bullet_left = round(sprite_x_location(bullet));
  int bullet_right = 2 - 1 + bullet_left;

  // If its mothership's bullet trying to collide with spacecraft
  if(s_craft == 1){
    int spaceCraft_top = round(sprite_y_location(alien));
    int spaceCraft_bottom = SPACECRAFT_HEIGHT - 1 + spaceCraft_top;
    int spaceCraft_left = round(sprite_x_location(alien));
    int spaceCraft_right = SPACECRAFT_WIDTH - 1 + spaceCraft_left;

    // If spacecraft is hit by the mothership's bullet
    if(!(bullet_top > spaceCraft_bottom || spaceCraft_top > bullet_bottom || bullet_left > spaceCraft_right || spaceCraft_left > bullet_right)){
      send_debug_string("Mothership destroyed player");
      // Hide bullet from screen
      make_sprite_invisible(bullet, 0);

      bullet_on_screen = 0; // Tell program bullet is not on screen anymore.
      init_spaceCraft();
      // Move bullet out of playfield so its not stuck on alien
      bullet->x = 0;
      bullet->y = 0;
      lives--;
      second_counter2 = 0; // Reset timer, so it takes 2 - 4 sec, when alien spawn to attack
      seed += 10;
      rn2 = random_number(); // Update the second it will shoot, so it wont be stuck on one number
    }
  }
  else{
    // If its alien on field
    if(boss_battle == 0){
      int alien_top = round(sprite_y_location(alien));
      int alien_bottom = ALIEN_HEIGHT - 1 + alien_top;
      int alien_left = round(sprite_x_location(alien));
      int alien_right = ALIEN_WIDTH - 1 + alien_left;

      if(!(bullet_top > alien_bottom || alien_top > bullet_bottom || bullet_left > alien_right || alien_left > bullet_right)){
        send_debug_string("Alien killed by the player");
        // Hide bullet from screen
        make_sprite_invisible(bullet, 0);
        // Tell program bullet is not on screen anymore.
        bullet_on_screen = 0;
        init_alien();
        // Move bullet out of playfield so its not stuck on alien
        bullet->x = 0;
        bullet->y = 0;
        attack = 0;
        alien_killed++;
        score++;
        second_counter = 0; // Reset timer, so it takes 2 - 4 sek, when alien spawn to attack
        seed += 10;
        rn = random_number(); // Update the second it will attack, so it wont be stuck on one number
      }
    }
    // If its mothership on field
    else if(boss_battle == 1){
      int mothership_top = round(sprite_y_location(alien)) + 1 ; // +1 because we dont want healthbar to be hit by bullet
      int mothership_bottom = MOTHERSHIP_HEIGHT - 1 + mothership_top;
      int mothership_left = round(sprite_x_location(alien));
      int mothership_right = MOTHERSHIP_WIDTH - 1 + mothership_left;

      // If bullet hits mothership
      if(!(bullet_top > mothership_bottom || mothership_top > bullet_bottom || bullet_left > mothership_right || mothership_left > bullet_right)){
        // Decrease mothership's lives
        mothership_lives--;
        update_healthbar(&mothership); // Changes the healthbar on mothership

        // Hide bullet from screen
        make_sprite_invisible(bullet, 0);
        // Tell program bullet is not on screen anymore.
        bullet_on_screen = 0;
        // Move bullet out of playfield so its not stuck on alien
        bullet->x = 0;
        bullet->y = 0;
        seed += 10;

      }
    }
  }
}

void intro_menu(){
  draw_string((LCD_X - gameTitleLength) / 2,y_top,gameTitle);
  // + 8 because char height is 8
  draw_string((LCD_X - firstNameLength) / 2,y_top + 8, firstName);
  draw_string((LCD_X - lastNameLength) / 2,y_top + 16, lastName);
  draw_string((LCD_X - stdNumLength) / 2,y_top + 24, stdNummer);
  draw_string((LCD_X - pressBtnLength) / 2,y_top + 32, pressBtnMsg);
  draw_string((LCD_X - pressBtnLength2) / 2,y_top + 40, pressBtnMsg2);

  show_screen();

  while(1){
    // If user press one of the buttons, start the countdown
    if((PINF>>6) & 0b1){
      debounce_PINF(6);
      start_count_down();
      // So it doesnt follow spaceCraft
      attack_spaceCraft(&alien, &spaceCraft, 0);
      break;
    }
    else if((PINF>>5) & 0b1){
      debounce_PINF(5);
      start_count_down();
      // So it doesnt follow spaceCraft
      attack_spaceCraft(&alien, &spaceCraft, 0);
      break;
    }
  }

  rn = random_number();
}

void start_count_down(){
  clear_screen();
  draw_string(x_center,y_center - 4, "3");
  show_screen();
  _delay_ms(300);
  draw_string(x_center,y_center - 4, "2");
  show_screen();
  _delay_ms(300);
  draw_string(x_center,y_center - 4, "1");
  show_screen();
  _delay_ms(300);

  // So the alien doesnt attack straight away, or at 1 second
  second_counter = 0;

  init_sprites();
  draw_game();

}

void draw_game(){
  clear_screen();
  draw_border();
  draw_status();
  draw_sprite(&spaceCraft);
  draw_sprite(&bullet);
  draw_sprite(&mothership_bullet);

  // Only show battleship if boss_battle is true
  if(boss_battle == 1){
    draw_sprite(&mothership);
  }
  else{
    draw_sprite(&alien);
  }

  show_screen();

}

void draw_border(){
  // Pluss 8 because we need room for texts ontop of the border
  // Top
  draw_line(x_left, y_top + 8, x_right, y_top + 8);

  // Bottom
  draw_line(x_left, y_bottom, x_right, y_bottom);

  // Right
  draw_line(x_right, y_bottom, x_right, y_top + 8);
  // Left
  draw_line(x_left, y_bottom, x_left, y_top + 8);
}

void draw_status(){
  char buffer [80];
  sprintf(buffer, "L:%d S:%d T:%d%d:%d%d", lives, score, min_minutes, minutes, sec_seconds, seconds);
  //sprintf(buffer, "%5.4f", curr_time);
  draw_string(0,0,buffer);
}

void update_timer(){
  //char buff_timer[40];
  int t = get_system_time(TCNT1);
  t = t * 10;
  if(t % 1 == 0){
    status_timer++;
  }

  if(status_timer % 30 == 0){
		seconds++;
	}
	if( sec_seconds == 5 && seconds > 9){
		minutes++;
		seconds = 0;
		sec_seconds = 0;
	}
	if(seconds > 9){
		sec_seconds++;
		seconds = 0;
	}
	if(minutes > 9){
		min_minutes = 0;
		minutes++;
	}
}

int random_number(){
  int random_num = rand() % 4 + 1;

  if(random_num == 1){
    random_num = 2;
  }
  else if(random_num == 0){
    random_num = 3;
  }

  return random_num;
}

void process_game(){
  char curr_char = usb_serial_getchar();

  // Timer
  update_timer();

  // Moves the sprite dependent on direction. Parameter only in use if the user uses the keyboard
  move_spaceCraft(curr_char);

  // Game over button
  // if((PINF>>5) & 0b1){
  //   debounce_PINF(5);
  //   game_over_screen();
  // }

  if((PINF>>6) & 0b1 || curr_char == 'f'){
    shoot_bullet(&bullet);
  }

  // Goes 1 second
  if(status_timer % 30 == 0){
    // Få randomnumber, eks 2. Så en annen variabel som blir telt til de er like, også kjør attack spaceCraft
    second_counter++;
    second_counter2++;

    if(second_counter == rn && boss_battle == 0){
      attack_spaceCraft(&alien, &spaceCraft, 0);
      attack = 1;
      // reset the counter, so it counts up to the new random number (rn)
      second_counter = 0;
    }
    else if(second_counter == rn && boss_battle == 1){
      attack_spaceCraft(&mothership, &spaceCraft, 1);
      attack = 1;
      second_counter = 0;
    }

    // Bullet for mothership
    if(second_counter2 == rn2 && boss_battle == 1){
      mothership_shoot_bullet(&mothership_bullet);
      second_counter2 = 0;
      rn2 = random_number();
    }

  }

  // Moves the alien
  if(attack == 1 && boss_battle == 0){
    move_alien(&alien);
    second_counter = 0; // So it wont dash again before hitting a border. Incase it takes more than 2 - 4 seconds
  }
  else if(attack == 1 && boss_battle == 1){
    move_alien(&mothership);
    second_counter = 0; // So it wont dash again before hitting a border. Incase it takes more than 2 - 4 seconds
  }

  // If alien killed x amount of times, spawn the mothership
  if(alien_killed == 5){
    sprite_new_position(&alien, 5, 0);
    boss_battle = 1;
    init_mothership();
    // sprite_new_position(&mothership, 60, 20);
    // lives = second_counter;
    // So this if statement doesnt loop
    alien_killed++;
    rn = random_number();
    rn2 = random_number();
    second_counter = 0;
    second_counter2 = 0;
  }

  // Moves the motherships bullet if it fires, and removes bullet if mothership dies
  // Must have this function before "check_mothership_lives" because then the bullet wont be removed when mothership dies
  update_bullet(&mothership_bullet, 1);

  check_mothership_lives(&mothership);

  if(lives == 0){
    game_over_screen();
  }

  update_spaceCraft_direction(&spaceCraft, spaceCraft_direction);

  update_bullet(&bullet, 0);


  if(boss_battle == 0){
    // Checks if bullet hit alien
    collision_bullet_alien(&bullet, &alien, 0);
  }
  else if(boss_battle == 1){
    collision_bullet_alien(&bullet, &mothership, 0);
  }

  // Check if mothership's bullet hits the spaceCraft
  collision_bullet_alien(&mothership_bullet, &spaceCraft, 1);

  // Send message to therminal about the Spacecraft's: position (x,y) and direction
  write_spacecraft_info_to_therminal();

  // Rembmer this one, draws the spacecraft. So it will refresh screen all the time.
  draw_game();
}

int main(void){
  setup_ports();

  // Runs until the connection with usb is done.
  while(!usb_configured() || !usb_serial_get_control());
  // Computer successfully detected
  send_debug_string("Welcome to Alien Advance");
  send_debug_string("Connection between teensy and computer is now active");
  send_debug_string("Keyboard controls:");
  send_debug_string("W: move up");
  send_debug_string("S: move down");
  send_debug_string("A: move left");
  send_debug_string("D: move right");
  send_debug_string("F: shoot");


  // Write message to teensy showing USB is connected. delay so user can see it
  draw_string((LCD_X - 13*5) / 2, y_center - 4, "USB connected");
  show_screen();
  _delay_ms(1000);

  clear_screen();
  intro_menu();
  draw_game();


  while(1){
      process_game();
  }

  return 0;
}

void game_over_screen(){
  // Set game over to 1(True)
  game_over = 1;
  clear_screen();
  draw_string((LCD_X - gameOverMsgLength) / 2, y_top + 8, gameOverMsg);
  draw_string((LCD_X - gameOverMsgLength2) / 2, y_top + 20, gameOverMsg2);
  draw_string((LCD_X - gameOverMsgLength3) / 2, y_top + 28, gameOverMsg3);
  draw_string((LCD_X - gameOverMsgLength4) / 2, y_top + 36, gameOverMsg4);
  show_screen();

  while(1){
    if((PINF>>6) & 0b1){
        debounce_PINF(6);
        restart_game();
        break;
    }
    else if((PINF>>5) & 0b1){
        debounce_PINF(5);
        restart_game();
        break;
    }
  }
}

void restart_game(){
  clear_screen();
  lives = 10;
  score = 0;
  status_timer = 0;
  game_over = 0;
  spaceCraft_direction = 0; // So it doesnt change direction, if you died while direction was left, for example.
  seconds = 0;
  sec_seconds = 0;
  minutes = 0;
  min_minutes = 0;
  alien_killed = 0;
  boss_battle = 0;
  second_counter = 0;
  second_counter2 = 0;
  mothership_lives = 10;
  attack = 0; // If kiled during alien is in dash attack, it doenst start the game with alien in attack

  intro_menu();
}

void debounce_PINF(int pin_number){
  _delay_ms(100);

  // Runs aslong as the button is pressed. Meanwhile we increase the seed for aslongs as the person holds the button
  while ((PINF>>pin_number) & 0b1){
    seed++;
  }
  _delay_ms(100);
}

// From ZDK library cab202
// Author: Lawrence Buckingham, Benjamin Talbot
void sprite_move(Sprite *sprite, int dx, int dy){
  sprite->x += dx;
  sprite->y += dy;
}

// From ZDK library cab202
// Author: Lawrence Buckingham, Benjamin Talbot
void sprite_turn_to(Sprite *sprite, double dx, double dy){
  sprite->dx = dx;
  sprite->dy = dy;
}

// Makes the sprite step one pixel each time, to its goal.
// From ZDK library cab202
// Author: Lawrence Buckingham, Benjamin Talbot
void sprite_step(Sprite *sprite){
  sprite->x += sprite->dx;
  sprite->y += sprite->dy;
}

// From ZDK library cab202
// Author: Lawrence Buckingham, Benjamin Talbot
int sprite_x_location(Sprite *sprite){
  int x = round(sprite->x);
  return x;
}

// From ZDK library cab202
// Author: Lawrence Buckingham, Benjamin Talbot
int sprite_y_location(Sprite *sprite){
  int y = round(sprite->y);
  return y;
}

void make_sprite_invisible(Sprite *sprite, int value){
  // If value is 1 make the sprite visible, otherwise, make it invisible
  if(value == 1){
    sprite->is_visible = 1;
  }
  else{
    sprite->is_visible = 0;
  }
}

void sprite_new_position(Sprite *sprite, int x, int y){
  sprite->x = x;
  sprite->y = y;
}

void send_debug_string(char* string){
  char buffer [50];

  double time = get_system_time_debugger();

  sprintf(buffer, "[DEBUG @ %3.3f] ", time);

  // Parameter two is amount of characters, make sure its the correct amount, or you will get strange symbols
  usb_serial_write(buffer, 17);

  // Send all of the characters in the string
  unsigned char char_count = 0;
  while (*string != '\0') {
      usb_serial_putchar(*string);
      string++;
      char_count++;
  }

  // Go to a new line (force this to be the start of the line)
  usb_serial_putchar('\r');
  usb_serial_putchar('\n');

}

void write_spacecraft_info_to_therminal(){
  // Reach 0.5 seconds. its bigger than, because == wont work here, overflow is to quick here
  if(timer0_overflow_counter > 50){
    int x = sprite_x_location(&spaceCraft);
    int y = sprite_y_location(&spaceCraft);

    char spaceCraft_location [50];
    char spaceCraft_curr_direction [50];

    sprintf(spaceCraft_location, "Spacecraft's location (%d,%d)", x, y);


    if(spaceCraft_direction == 0){
      sprintf(spaceCraft_curr_direction, "Spacecraft's current heading is up");
    }
    else if(spaceCraft_direction == 1){
      sprintf(spaceCraft_curr_direction, "Spacecraft's current heading is down");
    }
    else if(spaceCraft_direction == 2){
      sprintf(spaceCraft_curr_direction, "Spacecraft's current heading is left");
    }
    else if(spaceCraft_direction == 3){
      sprintf(spaceCraft_curr_direction, "Spacecraft's current heading is right");
    }

    send_debug_string(spaceCraft_location);
    send_debug_string(spaceCraft_curr_direction);

    timer0_overflow_counter = 0;
  }
}

ISR(TIMER0_COMPA_vect){
  timer0_overflow_counter++;

  // Checks if alien hit spaceCraft
  if(boss_battle == 0){
    collision(&spaceCraft, &alien);
  }
  else if(boss_battle == 1){
    collision(&spaceCraft, &mothership);
  }

}

ISR(TIMER1_OVF_vect){
  timer1_overflow_counter++;
}
