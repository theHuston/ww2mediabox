#include <Entropy.h>
#include <RBD_Timer.h>
#include <RBD_Button.h>
#include <ProTrinketHidCombo.h>



uint16_t *ptr;

//Scroll Pins
#define PIN_ENCODER_SCROLL_A      6
#define PIN_ENCODER_SCROLL_B      5
#define TRINKET_PINx       PIND

//vars for SCROLL
static uint8_t enc_prev_pos   = 0;
static uint8_t enc_flags      = 0;
static char    sw_was_pressed = 0;
static bool directionVertical = false;

//VOLUME Pins
#define PIN_ENCODER_VOLUME_A      4
#define PIN_ENCODER_VOLUME_B      3
#define VOLUME_TRINKET_PINx       PIND

//vars for VOLUME
static uint8_t vol_enc_prev_pos   = 0;
static uint8_t vol_enc_flags      = 0;


// Buttons and flicker led
const int PIN_BUTTON_DIRECTION_LED = 10;
const int PIN_BUTTON_DIRECTION_TOGGLE = 9;
const int PIN_BUTTON_BACKSPACE_CLICK = 11;
const int PIN_BUTTON_MOUSE_CLICK = 12;
const int PIN_LED_FLICKER = 13;


//Radial Switch
const int PIN_RADIAL_0 = 14;
const int PIN_RADIAL_1 = 8;
const int PIN_RADIAL_2 = 19;
const int PIN_RADIAL_3 = 18;
const int PIN_RADIAL_4 = 17;
const int PIN_RADIAL_5 = 16;
const int PIN_RADIAL_6 = 15;


RBD::Timer flickerSeqTimer;

static int flickTime = 50;
static int flickSeqTime = 2000;
static int flickBrightness = 60;

static int minFlickTime = 400;
static int maxFlickTime = 500;
static int minFlickBright = 120;
static int maxFlickBright = 135;


//Create buttons
RBD::Button directionalButton(PIN_BUTTON_DIRECTION_TOGGLE);

RBD::Button mouseButton(PIN_BUTTON_MOUSE_CLICK);
RBD::Button backButton(PIN_BUTTON_BACKSPACE_CLICK);

static bool firstRadial = false;
RBD::Button radialButton0(PIN_RADIAL_0);
RBD::Button radialButton1(PIN_RADIAL_1);
RBD::Button radialButton2(PIN_RADIAL_2);
RBD::Button radialButton3(PIN_RADIAL_3);
RBD::Button radialButton4(PIN_RADIAL_4);
RBD::Button radialButton5(PIN_RADIAL_5);
RBD::Button radialButton6(PIN_RADIAL_6);

void setup()
{
  Serial.begin(9600);
  Serial.println("WWII MediaBox:");

  //ptr = ms_init(CMA);
  //if(ptr == NULL) Serial.println("No memory");

  Entropy.initialize();
  setupSequence();

  // set pins as input with internal pull-up resistors enabled
  pinMode(PIN_ENCODER_SCROLL_A, INPUT_PULLUP);
  pinMode(PIN_ENCODER_SCROLL_B, INPUT_PULLUP);

  // set pins as input with internal pull-up resistors enabled
  pinMode(PIN_ENCODER_VOLUME_A, INPUT);
  pinMode(PIN_ENCODER_VOLUME_B, INPUT);
  digitalWrite(PIN_ENCODER_VOLUME_A, HIGH);
  digitalWrite(PIN_ENCODER_VOLUME_B, HIGH);

  // start USB stuff
  TrinketHidCombo.begin();

  // get an initial reading on the encoder pins
  if (digitalRead(PIN_ENCODER_SCROLL_A) == LOW) {
    enc_prev_pos |= (1 << 0);
  }
  if (digitalRead(PIN_ENCODER_SCROLL_B) == LOW) {
    enc_prev_pos |= (1 << 1);
  }

  // get an initial reading on the encoder pins
  if (digitalRead(PIN_ENCODER_VOLUME_A) == LOW) {
    vol_enc_prev_pos |= (1 << 0);
  }
  if (digitalRead(PIN_ENCODER_VOLUME_B) == LOW) {
    vol_enc_prev_pos |= (1 << 1);
  }

}

void setupSequence()
{

  flickerSeqTimer.setTimeout(flickSeqTime);
  flickerSeqTimer.restart();

}

void flicker()
{
  int ran = Entropy.random( 1, 9 );
  int bright = 0;

  if ( ran <= 4 )
  {
    // NORMAL RUNNING AVARAGE DISPLAY
    minFlickBright = Entropy.random(100, 140);
    maxFlickBright = 255 - minFlickBright;

    flickSeqTime = Entropy.random(3000, 6000);

  } else if ( ran >= 5 && ran < 6 ) {

    // DIFFERENT RUNNING DISPLAY
    minFlickBright = Entropy.random(80, 140);
    maxFlickBright = 255 - minFlickBright;

    flickSeqTime = Entropy.random(3000, 4000);

  } else {

    // WILD DIFFERENT RUNNING DISPLAY
    minFlickBright = Entropy.random(20, 255);
    maxFlickBright = 255 - minFlickBright;

    flickSeqTime = Entropy.random(500, 1500);
  }

  setupSequence();
  flickerSeqTimer.restart();
}

void loop()
{
  analogWrite(PIN_LED_FLICKER, random(minFlickBright) + maxFlickBright);

  if (flickerSeqTimer.isExpired()) {
    flicker();
  }

  //START SCROLL
  int8_t enc_action = 0; // 1 or -1 if moved, sign is direction

  // note: for better performance, the code will use
  // direct port access techniques
  // http://www.arduino.cc/en/Reference/PortManipulation

  uint8_t enc_cur_pos = 0;
  // read in the encoder state first
  if (bit_is_clear(TRINKET_PINx, PIN_ENCODER_SCROLL_A)) {
    enc_cur_pos |= (1 << 0);
  }
  if (bit_is_clear(TRINKET_PINx, PIN_ENCODER_SCROLL_B)) {
    enc_cur_pos |= (1 << 1);
  }

  // if any rotation at all
  if (enc_cur_pos != enc_prev_pos)
  {
    if (enc_prev_pos == 0x00)
    {
      // this is the first edge
      if (enc_cur_pos == 0x01) {
        enc_flags |= (1 << 0);
      }
      else if (enc_cur_pos == 0x02) {
        enc_flags |= (1 << 1);
      }
    }

    if (enc_cur_pos == 0x03)
    {
      // this is when the encoder is in the middle of a "step"
      enc_flags |= (1 << 4);
    }
    else if (enc_cur_pos == 0x00)
    {
      // this is the final edge
      if (enc_prev_pos == 0x02) {
        enc_flags |= (1 << 2);
      }
      else if (enc_prev_pos == 0x01) {
        enc_flags |= (1 << 3);
      }

      // check the first and last edge
      // or maybe one edge is missing, if missing then require the middle state
      // this will reject bounces and false movements
      if (bit_is_set(enc_flags, 0) && (bit_is_set(enc_flags, 2) || bit_is_set(enc_flags, 4))) {
        enc_action = 1;
      }
      else if (bit_is_set(enc_flags, 2) && (bit_is_set(enc_flags, 0) || bit_is_set(enc_flags, 4))) {
        enc_action = 1;
      }
      else if (bit_is_set(enc_flags, 1) && (bit_is_set(enc_flags, 3) || bit_is_set(enc_flags, 4))) {
        enc_action = -1;
      }
      else if (bit_is_set(enc_flags, 3) && (bit_is_set(enc_flags, 1) || bit_is_set(enc_flags, 4))) {
        enc_action = -1;
      }

      enc_flags = 0; // reset for next time
    }
  }

  enc_prev_pos = enc_cur_pos;

  if (enc_action > 0) {

    if (directionVertical) {
      TrinketHidCombo.pressKey(0, KEYCODE_ARROW_DOWN);// Clockwise, send multimedia ARROW down
      TrinketHidCombo.pressKey(0, 0);
    } else {
      TrinketHidCombo.pressKey(0, KEYCODE_ARROW_RIGHT);// Clockwise, send multimedia ARROW right
      TrinketHidCombo.pressKey(0, 0);
    }
  }
  else if (enc_action < 0) {

    if (directionVertical) {
      TrinketHidCombo.pressKey(0, KEYCODE_ARROW_UP); // Counterclockwise, is multimedia ARROW up
      TrinketHidCombo.pressKey(0, 0);
    } else {
      TrinketHidCombo.pressKey(0, KEYCODE_ARROW_LEFT);// Clockwise, send multimedia ARROW left
      TrinketHidCombo.pressKey(0, 0);
    }
  }


  // START VOLUME
  int8_t vol_enc_action = 0; // 1 or -1 if moved, sign is direction

  // note: for better performance, the code will use
  // direct port access techniques
  // http://www.arduino.cc/en/Reference/PortManipulation
  uint8_t vol_enc_cur_pos = 0;
  // read in the encoder state first
  if (bit_is_clear(VOLUME_TRINKET_PINx, PIN_ENCODER_VOLUME_A)) {
    vol_enc_cur_pos |= (1 << 0);
  }
  if (bit_is_clear(VOLUME_TRINKET_PINx, PIN_ENCODER_VOLUME_B)) {
    vol_enc_cur_pos |= (1 << 1);
  }

  // if any rotation at all
  if (vol_enc_cur_pos != vol_enc_prev_pos)
  {
    if (vol_enc_prev_pos == 0x00)
    {
      // this is the first edge
      if (vol_enc_cur_pos == 0x01) {
        vol_enc_flags |= (1 << 0);
      }
      else if (vol_enc_cur_pos == 0x02) {
        vol_enc_flags |= (1 << 1);
      }
    }

    if (vol_enc_cur_pos == 0x03)
    {
      // this is when the encoder is in the middle of a "step"
      vol_enc_flags |= (1 << 4);
    }
    else if (vol_enc_cur_pos == 0x00)
    {
      // this is the final edge
      if (vol_enc_prev_pos == 0x02) {
        vol_enc_flags |= (1 << 2);
      }
      else if (vol_enc_prev_pos == 0x01) {
        vol_enc_flags |= (1 << 3);
      }

      // check the first and last edge
      // or maybe one edge is missing, if missing then require the middle state
      // this will reject bounces and false movements
      if (bit_is_set(vol_enc_flags, 0) && (bit_is_set(vol_enc_flags, 2) || bit_is_set(vol_enc_flags, 4))) {
        vol_enc_action = 1;
      }
      else if (bit_is_set(vol_enc_flags, 2) && (bit_is_set(vol_enc_flags, 0) || bit_is_set(vol_enc_flags, 4))) {
        vol_enc_action = 1;
      }
      else if (bit_is_set(vol_enc_flags, 1) && (bit_is_set(vol_enc_flags, 3) || bit_is_set(vol_enc_flags, 4))) {
        vol_enc_action = -1;
      }
      else if (bit_is_set(vol_enc_flags, 3) && (bit_is_set(vol_enc_flags, 1) || bit_is_set(vol_enc_flags, 4))) {
        vol_enc_action = -1;
      }

      vol_enc_flags = 0; // reset for next time
    }
  }

  vol_enc_prev_pos = vol_enc_cur_pos;

  if (vol_enc_action > 0) {
    TrinketHidCombo.pressMultimediaKey(MMKEY_VOL_DOWN);
  }
  else if (vol_enc_action < 0) {
    TrinketHidCombo.pressMultimediaKey(MMKEY_VOL_UP);
  }

  TrinketHidCombo.poll(); // do nothing, check if USB needs anything done


  // the poll function must be called at least once every 10 ms
  // or cause a keystroke
  // if it is not, then the computer may think that the device
  // has stopped working, and give errors

  if (directionalButton.onReleased()) {
    if (directionVertical) {
      directionVertical = false;
      digitalWrite(PIN_BUTTON_DIRECTION_LED, LOW);  // toggle LED to indicate button state
    } else {
      directionVertical = true;
      digitalWrite(PIN_BUTTON_DIRECTION_LED, HIGH);  // toggle LED to indicate button state
    }

  }

  if (mouseButton.onPressed()) {
    TrinketHidCombo.pressKey(0, KEYCODE_ENTER);
  }

  if (mouseButton.onReleased()) {
    TrinketHidCombo.pressKey(0, 0);
  }

  if (backButton.onPressed()) {
    TrinketHidCombo.pressKey(0, KEYCODE_BACKSPACE);
    // this should type a backspace
  }

  if (backButton.onReleased()) {
    TrinketHidCombo.pressKey(0, 0);
    // this releases the key
  }


  //RADIAL DIAL BUTTONS ////////////////////////////////////

  //Radial 0
  if (radialButton0.onPressed()) {
    if (firstRadial) {
      TrinketHidCombo.pressKey(0, KEYCODE_S);
      TrinketHidCombo.pressKey(0, 0);
      Serial.println("Keypress F0");
    } else {
      firstRadial = true;
    }
  }

  //Radial 1
  if (radialButton1.onPressed()) {
    if (firstRadial) {
      TrinketHidCombo.pressKey(0, KEYCODE_F1);
      TrinketHidCombo.pressKey(0, 0);
      Serial.println("Keypress F1");
    } else {
      firstRadial = true;
    }
  }

  //Radial 2
  if (radialButton2.onPressed()) {
    if (firstRadial) {
      TrinketHidCombo.pressKey(0, KEYCODE_F2);
      TrinketHidCombo.pressKey(0, 0);
      Serial.println("Keypress F2");
    } else {
      firstRadial = true;
    }
  }

  //Radial 3
  if (radialButton3.onPressed()) {
    if (firstRadial) {
      TrinketHidCombo.pressKey(0, KEYCODE_F3);
      TrinketHidCombo.pressKey(0, 0);
      Serial.println("Keypress F3");
    } else {
      firstRadial = true;
    }
  }

  //Radial 4
  if (radialButton4.onPressed()) {
    if (firstRadial) {
      TrinketHidCombo.pressKey(0, KEYCODE_F4);
      TrinketHidCombo.pressKey(0, 0);
      Serial.println("Keypress F4");
    } else {
      firstRadial = true;
    }
  }

  //Radial 5
  if (radialButton5.onPressed()) {
    if (firstRadial) {
      TrinketHidCombo.pressKey(0, KEYCODE_F5);
      TrinketHidCombo.pressKey(0, 0);
      Serial.println("Keypress F5");
    } else {
      firstRadial = true;
    }
  }

  //Radial 6
  if (radialButton6.onPressed()) {
    if (firstRadial) {
      TrinketHidCombo.pressKey(0, KEYCODE_F6);
      TrinketHidCombo.pressKey(0, 0);
      Serial.println("Keypress F6");
    } else {
      firstRadial = true;
    }
  }


}
