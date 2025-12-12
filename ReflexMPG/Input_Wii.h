/*******************************************************************************
 * Wii input module for RetroZord / Reflex-Adapt.
 * By Matheus Fraguas (sonik-br)
 * 
 * Handles a single input port.
 * 
 * Works with?.
 *
 * Uses NintendoExtensionCtrl Lib
 * https://github.com/dmadison/NintendoExtensionCtrl/
 * 
 * Uses SoftWire Lib
 * https://github.com/felias-fogg/SoftI2CMaster
 *
 * Uses a modified version of MPG
 * https://github.com/sonik-br/MPG
 *
*/

//Wii pins - Port 1

//SoftWire config
//Also configured in:
//NXC_Comms.h

#define SCL_PIN 5// D9/A9/PB5
#define SCL_PORT PORTB
#define SDA_PIN 4// D8/A8/PB4
#define SDA_PORT PORTB

#define I2C_FASTMODE 0
#define I2C_TIMEOUT 1000
#define I2C_PULLUP 1

#include "RZInputModule.h"
#include "src/SoftWire/SoftWire.h"
#include "src/NintendoExtensionCtrl/NintendoExtensionCtrl.h"

// Port 2 software I2C (separate pins)
#undef SCL_PIN
#undef SCL_PORT
#undef SDA_PIN
#undef SDA_PORT
#define SCL_PIN 2  // D16/PB2 on 32u4
#define SCL_PORT PORTB
#define SDA_PIN 3  // D14/PB3 on 32u4
#define SDA_PORT PORTB

// Ensure SoftWire2 brings in its implementation
#undef USE_SOFTWIRE_H_AS_PLAIN_INCLUDE
#include "src/SoftWire/SoftWire2.h"

#define WII_ANALOG_DEFAULT_CENTER 127U

class ReflexInputWii : public RZInputModule {
  private:
    ExtensionPort* wii[2];
    ClassicController::Shared* wii_classic[2];
    Nunchuk::Shared* wii_nchuk[2];
    #ifdef ENABLE_WII_GUITAR
      GuitarController::Shared* wii_guitar[2];
    #endif

    bool needDetectAnalogTrigger[2];

    void wiiResetAnalogMinMax() {
      //n64_x_min = -50;
      //n64_x_max = 50;
      //n64_y_min = -50;
      //n64_y_max = 50;
    }
    
    void wiiResetJoyValues(const uint8_t i) {
      if (i >= totalUsb)
        return;
      resetState(i);
      wiiResetAnalogMinMax();
    }

    #ifdef ENABLE_REFLEX_PAD
      const Pad padWii[15] = {
        {     1,  2, 8*6, FACEBTN_ON, FACEBTN_OFF }, //Y
        {  1<<1,  3, 9*6, FACEBTN_ON, FACEBTN_OFF }, //B
        {  1<<2,  2, 10*6, FACEBTN_ON, FACEBTN_OFF }, //A
        {  1<<3,  1, 9*6, FACEBTN_ON, FACEBTN_OFF }, //X
        {  1<<4,  0, 0*6, SHOULDERBTN_ON, SHOULDERBTN_OFF }, //L
        {  1<<5,  0, 8*6, SHOULDERBTN_ON, SHOULDERBTN_OFF }, //R
        {  1<<6,  0, 2*6, SHOULDERBTN_ON, SHOULDERBTN_OFF }, //ZL
        {  1<<7,  0, 10*6, SHOULDERBTN_ON, SHOULDERBTN_OFF }, //ZR
        {  1<<8,  2, 4*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF }, //MINUS
        {  1<<9,  2, 6*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF }, //PLUS
        { 1<<10,  2, 5*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF }, //HOME
        { 1<<11,  1, 1*6, UP_ON, UP_OFF },
        { 1<<12,  3, 1*6, DOWN_ON, DOWN_OFF },
        { 1<<13,  2, 0,   LEFT_ON, LEFT_OFF },
        { 1<<14,  2, 2*6, RIGHT_ON, RIGHT_OFF }
      };
    
      void ShowDefaultPadWii(const uint8_t index, const ExtensionType padType) {
        display.clear(padDivision[index].firstCol, padDivision[index].lastCol, oledDisplayFirstRow + 1, 7);
        display.setCursor(padDivision[index].firstCol, 7);
        
        switch(padType) {
          case(ExtensionType::Nunchuk):
            display.print(F("NUNCHUK"));
            break;
          case(ExtensionType::ClassicController):
            display.print(F("CLASSIC"));
            break;
    
          //non-supported devices will still display it's name
          case(ExtensionType::GuitarController):
            display.print(F("GUITAR"));
            return;
          case(ExtensionType::DrumController):
            display.print(F("DRUM"));
            return;
          case(ExtensionType::DJTurntableController):
            display.print(F("TURNTABLE"));
            return;
          case(ExtensionType::uDrawTablet):
            display.print(F("UDRAW"));
            return;
          case(ExtensionType::DrawsomeTablet):
            display.print(F("DRAWSOME"));
            return;
          case(ExtensionType::UnknownController):
            display.print(F("NOT SUPPORTED"));
            return;
          default:
            display.print(PSTR_TO_F(PSTR_NONE));
            return;
        }

        if (index < 2) {
          if(padType == ExtensionType::Nunchuk) {
            for(uint8_t x = 0; x < 2; ++x){
              const Pad pad = padWii[x];
              PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, true, pad.on, pad.off, true);
            }
          } else { //classic
            for(uint8_t x = 0; x < (sizeof(padWii) / sizeof(Pad)); ++x){
              const Pad pad = padWii[x];
              PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, true, pad.on, pad.off, true);
            }
          }
        }
      }
    #endif

  public:
    ReflexInputWii() : RZInputModule() { }

    const char* getUsbId() override {
      static const char* usbId { "RZMWii" };
      return usbId;
    }

    const uint16_t getUsbVersion() override {
      return MODE_ID_WII;
    }

//    #ifdef ENABLE_REFLEX_PAD
//      void displayInputName() override {
//        display.setCol(9*6);
//        display.print(F("WII"));
//      }
//    #endif



    void setup() override {
      delay(5);
    
      //SoftWire initialization
      Wire.begin();
      Wire2.begin();
    
      //Will use 400khz if I2C_FASTMODE 1.
      //Wire.setClock(100000UL);
      //Wire.setClock(400000UL);
    
      totalUsb = 2;

      wii[0] = new ExtensionPort(Wire);
      wii[1] = new ExtensionPort(Wire2);
      for (uint8_t i = 0; i < totalUsb; ++i) {
        wii_classic[i] = new ClassicController::Shared(*wii[i]);
        wii_nchuk[i] = new Nunchuk::Shared(*wii[i]);
        #ifdef ENABLE_WII_GUITAR
          wii_guitar[i] = new GuitarController::Shared(*wii[i]);
        #endif
        wii[i]->begin();
        hasLeftAnalogStick[i] = true;
        hasRightAnalogStick[i] = true;
        needDetectAnalogTrigger[i] = true;
      }

      delay(50);
    }

    void setup2() override {
      for (uint8_t i = 0; i < totalUsb; ++i) {
        needDetectAnalogTrigger[i] = true;
      }
    }

    bool read() override {
      static boolean haveController[2] = { false, false };
      static bool isClassicAnalog[2] = { false, false };
      static uint16_t oldButtons[2] = { 0, 0 };
      static uint8_t oldHat[2] = { 0, 0 };
      static uint8_t oldLX[2] = { WII_ANALOG_DEFAULT_CENTER, WII_ANALOG_DEFAULT_CENTER };
      static uint8_t oldLY[2] = { WII_ANALOG_DEFAULT_CENTER, WII_ANALOG_DEFAULT_CENTER };
      static uint8_t oldRX[2] = { WII_ANALOG_DEFAULT_CENTER, WII_ANALOG_DEFAULT_CENTER };
      static uint8_t oldRY[2] = { WII_ANALOG_DEFAULT_CENTER, WII_ANALOG_DEFAULT_CENTER };
      static uint8_t oldLT[2] = { 0, 0 };
      static uint8_t oldRT[2] = { 0, 0 };

      #ifndef WII_ANALOG_RAW
        const uint8_t wii_analog_stick_min = 25;
        const uint8_t wii_analog_stick_max = 230;
        const uint8_t wii_analog_trigger_min = 14;
        static uint8_t wii_lx_shift[2] = { 0, 0 };
        static uint8_t wii_ly_shift[2] = { 0, 0 };
        static uint8_t wii_rx_shift[2] = { 0, 0 };
        static uint8_t wii_ry_shift[2] = { 0, 0 };
      #endif

      #ifdef ENABLE_REFLEX_PAD
        static ExtensionType lastPadType[] = { ExtensionType::UnknownController, ExtensionType::UnknownController };
        ExtensionType currentPadType[] = { ExtensionType::NoController, ExtensionType::NoController };
        static bool firstTime = true;
        if(firstTime) {
          firstTime = false;
          for (uint8_t i = 0; i < totalUsb; ++i) {
            ShowDefaultPadWii(i, ExtensionType::NoController);
          }
        }
      #endif

      bool stateChanged = false;
      bool anyController = false;

      for (uint8_t i = 0; i < totalUsb; ++i) {
        ExtensionPort* port = wii[i];
        ClassicController::Shared* classic = wii_classic[i];
        Nunchuk::Shared* nchuk = wii_nchuk[i];
        #ifdef ENABLE_WII_GUITAR
          GuitarController::Shared* guitar = wii_guitar[i];
        #endif

        if (!haveController[i]) {
          if (port->connect()) {
            haveController[i] = true;
            needDetectAnalogTrigger[i] = true;
            isClassicAnalog[i] = false;
            wiiResetJoyValues(i);
            #ifdef ENABLE_REFLEX_PAD
              ShowDefaultPadWii(i, port->getControllerType());
            #endif
          }
          continue;
        }

        if (!port->update()) {
          haveController[i] = false;
          hasAnalogTriggers[i] = false;
          wiiResetJoyValues(i);
          oldButtons[i] = 0;
          #ifdef ENABLE_REFLEX_PAD
            ShowDefaultPadWii(i, ExtensionType::NoController);
          #endif
          continue;
        }

        anyController = true;

        uint16_t buttonData = 0;
        uint8_t hatData = 0;
        uint8_t leftX = 0;
        uint8_t leftY = 0;
        uint8_t rightX = 0;
        uint8_t rightY = 0;
        uint8_t lt = 255;
        uint8_t rt = 255;

        const ExtensionType conType = port->getControllerType();

        if(conType == ExtensionType::ClassicController || conType == ExtensionType::Nunchuk
        #ifdef ENABLE_WII_GUITAR
          || conType == ExtensionType::GuitarController
        #endif
        ) {

          if(conType == ExtensionType::ClassicController) {
            bitWrite(hatData, 0, !classic->dpadUp());
            bitWrite(hatData, 1, !classic->dpadDown());
            bitWrite(hatData, 2, !classic->dpadLeft());
            bitWrite(hatData, 3, !classic->dpadRight());
            
            bitWrite(buttonData, 0, classic->buttonY());
            bitWrite(buttonData, 1, classic->buttonB());
            bitWrite(buttonData, 2, classic->buttonA());
            bitWrite(buttonData, 3, classic->buttonX());
            bitWrite(buttonData, 4, classic->buttonL());
            bitWrite(buttonData, 5, classic->buttonR());
            bitWrite(buttonData, 6, classic->buttonZL());
            bitWrite(buttonData, 7, classic->buttonZR());
            bitWrite(buttonData, 8, classic->buttonMinus());
            bitWrite(buttonData, 9, classic->buttonPlus());
            bitWrite(buttonData, 10, classic->buttonHome());

            leftX = classic->leftJoyX();
            leftY = classic->leftJoyY();
            rightX = classic->rightJoyX();
            rightY = classic->rightJoyY();
            lt = classic->triggerL();
            rt = classic->triggerR();


            if (needDetectAnalogTrigger[i]) {
              needDetectAnalogTrigger[i] = false;

            #ifndef WII_ANALOG_RAW
              wii_lx_shift[i] = (leftX >= 127) ? (leftX - 127) : (127 - leftX);
              wii_ly_shift[i] = (leftY >= 127) ? (leftY - 127) : (127 - leftY);
              wii_rx_shift[i] = (rightX >= 127) ? (rightX - 127) : (127 - rightX);
              wii_ry_shift[i] = (rightY >= 127) ? (rightY - 127) : (127 - rightY);
            #endif


              isClassicAnalog[i] = !(lt == 0 || rt == 0);
              if (isClassicAnalog[i] && canUseAnalogTrigger()) {
                hasAnalogTriggers[i] = true;
              }
            }

            state[i].dpad = 0
              | (classic->dpadUp()    ? GAMEPAD_MASK_UP    : 0)
              | (classic->dpadDown()  ? GAMEPAD_MASK_DOWN  : 0)
              | (classic->dpadLeft()  ? GAMEPAD_MASK_LEFT  : 0)
              | (classic->dpadRight() ? GAMEPAD_MASK_RIGHT : 0);

            state[i].buttons = 0
              | (classic->buttonB()  ? GAMEPAD_MASK_B1 : 0)
              | (classic->buttonA()  ? GAMEPAD_MASK_B2 : 0)
              | (classic->buttonY()  ? GAMEPAD_MASK_B3 : 0)
              | (classic->buttonX()  ? GAMEPAD_MASK_B4 : 0)
              | (classic->buttonMinus() ? GAMEPAD_MASK_S1 : 0)
              | (classic->buttonPlus() ? GAMEPAD_MASK_S2 : 0)
              | (classic->buttonHome() ? GAMEPAD_MASK_A1 : 0);

            if (isClassicAnalog[i]) {
              state[i].buttons = state[i].buttons
                | (classic->buttonZL() ? GAMEPAD_MASK_L1 : 0)
                | (classic->buttonZR() ? GAMEPAD_MASK_R1 : 0)
                | (classic->buttonL()  ? GAMEPAD_MASK_L2 : 0)
                | (classic->buttonR()  ? GAMEPAD_MASK_R2 : 0);
            } else {
              state[i].buttons = state[i].buttons
                | (classic->buttonL()  ? GAMEPAD_MASK_L1 : 0)
                | (classic->buttonR()  ? GAMEPAD_MASK_R1 : 0)
                | (classic->buttonZL() ? GAMEPAD_MASK_L2 : 0)
                | (classic->buttonZR() ? GAMEPAD_MASK_R2 : 0);
            }

#ifdef ENABLE_WII_GUITAR
          } else if (conType == ExtensionType::GuitarController) {
            bitWrite(buttonData, 0, guitar->fretBlue());
            bitWrite(buttonData, 1, guitar->fretRed());
            bitWrite(buttonData, 2, guitar->fretGreen());
            bitWrite(buttonData, 3, guitar->fretYellow());
            bitWrite(buttonData, 4, guitar->fretOrange());
            
            bitWrite(buttonData, 8, classic->buttonMinus());
            bitWrite(buttonData, 9, classic->buttonPlus());
  
            bitWrite(hatData, 0, !guitar->strumUp());
            bitWrite(hatData, 1, !guitar->strumDown());
            bitWrite(hatData, 2, 1);
            bitWrite(hatData, 3, 1);
  
            if (guitar->supportsTouchbar()) {
              bitWrite(buttonData, 5, guitar->touchBlue());
              bitWrite(buttonData, 6, guitar->touchRed());
              bitWrite(buttonData, 7, guitar->touchGreen());
              bitWrite(buttonData, 10, guitar->touchYellow());
              bitWrite(buttonData, 11, guitar->touchOrange());
            }
  
            leftX = map(guitar->joyX(), 0, 63, 0, 255);
            leftY = map(guitar->joyY(), 0, 63, 0, 255);
  
            rightX = map(guitar->whammyBar(), 15, 25, 0, 255);
#endif //ENABLE_WII_GUITAR
  
          } else { //nunchuk
            bitWrite(buttonData, 0, nchuk->buttonC());
            bitWrite(buttonData, 1, nchuk->buttonZ());
            state[i].buttons = 0
              | (nchuk->buttonC() ? GAMEPAD_MASK_B1 : 0)
              | (nchuk->buttonZ() ? GAMEPAD_MASK_B2 : 0);
  
            leftX = nchuk->joyX();
            leftY = nchuk->joyY();
          }
          
        }//end if classiccontroller / nunckuk

        bool buttonsChanged = buttonData != oldButtons[i] || hatData != oldHat[i];
        bool analogChanged = (leftX != oldLX[i]) || (leftY != oldLY[i]) || (rightX != oldRX[i]) || (rightY != oldRY[i]) || (lt != oldLT[i]) || (rt != oldRT[i]);

        if (buttonsChanged || analogChanged) { //state changed?
          stateChanged = stateChanged || buttonsChanged;

          #ifndef WII_ANALOG_RAW
            leftX -= wii_lx_shift[i];
            leftY -= wii_ly_shift[i];
            rightX -= wii_rx_shift[i];
            rightY -= wii_ry_shift[i];
            
            if (leftX < wii_analog_stick_min) leftX = wii_analog_stick_min;
            else if (leftX > wii_analog_stick_max) leftX = wii_analog_stick_max;
            if (leftY < wii_analog_stick_min) leftY = wii_analog_stick_min;
            else if (leftY > wii_analog_stick_max) leftY = wii_analog_stick_max;

            if (rightX < wii_analog_stick_min) rightX = wii_analog_stick_min;
            else if (rightX > wii_analog_stick_max) rightX = wii_analog_stick_max;
            if (rightY < wii_analog_stick_min) rightY = wii_analog_stick_min;
            else if (rightY > wii_analog_stick_max) rightY = wii_analog_stick_max;

            leftX =  map(leftX,  wii_analog_stick_min, wii_analog_stick_max, 0, 255);
            leftY =  map(leftY,  wii_analog_stick_min, wii_analog_stick_max, 0, 255);
            rightX = map(rightX, wii_analog_stick_min, wii_analog_stick_max, 0, 255);
            rightY = map(rightY, wii_analog_stick_min, wii_analog_stick_max, 0, 255);

            if (isClassicAnalog[i]) {
              if (lt < wii_analog_trigger_min) lt = 0;
              if (rt < wii_analog_trigger_min) rt = 0;
              lt = classic->buttonL() ? 0xFF : lt;
              rt = classic->buttonR() ? 0xFF : rt;
            }
          #endif

          state[i].lx = convertAnalog( leftX);
          state[i].ly = convertAnalog((uint8_t)~leftY);
          state[i].rx = convertAnalog( rightX);
          state[i].ry = convertAnalog((uint8_t)~rightY);
          state[i].lt = lt;
          state[i].rt = rt;

          #ifdef ENABLE_REFLEX_PAD
            const uint8_t nbuttons = (conType == ExtensionType::ClassicController || conType == ExtensionType::GuitarController) ? 15 : 2;
            for(uint8_t x = 0; x < nbuttons; ++x){
              const Pad pad = padWii[x];
              if(x < 11) //buttons
                PrintPadChar(i, padDivision[i].firstCol, pad.col, pad.row, pad.padvalue, bitRead(buttonData, x), pad.on, pad.off);
              else //dpad
                PrintPadChar(i, padDivision[i].firstCol, pad.col, pad.row, pad.padvalue, !bitRead(hatData, x-11), pad.on, pad.off);
            }
          #endif
          
          oldButtons[i] = buttonData;
          oldHat[i] = hatData;
          oldLX[i] = leftX;
          oldLY[i] = leftY;
          oldRX[i] = rightX;
          oldRY[i] = rightY;
          oldLT[i] = lt;
          oldRT[i] = rt;
        }//end if statechanged

      }//end bus loop

      sleepTime = anyController ? DEFAULT_SLEEP_TIME : 50000;

      return stateChanged;
    }//end read

};
