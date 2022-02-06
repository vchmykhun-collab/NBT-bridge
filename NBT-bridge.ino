#include "Arduino.h"
#include "lin.h"

Lin lin;

#include <SPI.h>
#include <mcp2515.h>

MCP2515 mcpNBT(8);
MCP2515 mcpFEM(10);

unsigned long refresh20_timer;
unsigned long refresh100_timer;
unsigned long refresh500_timer;
unsigned long refresh1000_timer;
unsigned long idle_timer;

unsigned long btns_timer;

//Required arbids
//0x130
//0x1A1
//0x1C2
//0x1C6
//0x1D6
//0x202
//0x21A
//0x23A
//0x26E
//0x286
//0x2B4
//0x2F8
//0x336
//0x367

struct can_frame can130;//ign on
struct can_frame can12f;//nbt wake-up
struct can_frame can12f_off;//nbt wake-up

struct can_frame can1a1;//speed
struct can_frame can1c2;//PDC distance
struct can_frame can1c6;//PDC acoustic

struct can_frame can4e4;//PDC something

struct can_frame can37a;

struct can_frame can202;//dimm
struct can_frame can2f7;//lang
struct can_frame can510;//idrive lag

struct can_frame can2a0;//trunk;
struct can_frame can1f6;//alarm light;
struct can_frame can6f1;

struct can_frame can380;

struct can_frame canPanel;
struct can_frame canMsg;

bool idle;
bool turned_off;

void enableBacklight(bool enable) 
{
  if (enable) {
    uint8_t linCmd[] = { 0xFF, 0xFE, 0xA5, 0xFF };  
    lin.send(0x2b,linCmd,4,0); 
  } else {
    uint8_t linCmd[] = { 0xFF, 0x00, 0xA5, 0xFF };  
    lin.send(0x2b,linCmd,4,0);     
  }
}

void audioPanelBacklight(uint8_t brightness) 
{
  uint8_t linCmd[] = { 0xFF, brightness, 0xA5, 0xFF };  
  lin.send(0x2b,linCmd,4,0); 
}

void setup(void)
{
  lin.begin(19200);
  audioPanelBacklight(0xfe);
 
  mcpNBT.reset();
  mcpNBT.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcpNBT.setNormalMode();

  mcpFEM.reset();
  mcpFEM.setBitrate(CAN_100KBPS, MCP_8MHZ);

//  mcpFEM.setFilterMask(0,0,0xf09); 
//  mcpFEM.setFilter(0,0,0x100);
//
//  mcpFEM.setFilterMask(1,0,0xe00);
//  mcpFEM.setFilter(1,0,0x200);

  mcpFEM.setFilterMask(0,0,0xfd39); 
  mcpFEM.setFilter(0,0,0x0121);

  mcpFEM.setFilterMask(1,0,0xfc01);
  mcpFEM.setFilter(1,0,0x0000);

  
  mcpFEM.setNormalMode();

  refresh20_timer = millis();
  refresh100_timer = millis();
  refresh500_timer = millis();
  refresh1000_timer = millis();

  idle = false;

  // nbt wake
  can12f.can_id  = 0x12f;
  can12f.can_dlc = 8;
  can12f.data[0] = 0xb7;//b7
  can12f.data[1] = 0x5a;//5a
  can12f.data[2] = 0x8a;
  can12f.data[3] = 0xdd;
  can12f.data[4] = 0xf4;
  can12f.data[5] = 0x05;
  can12f.data[6] = 0x30;
  can12f.data[7] = 0x01;

  // nbt wake
  //        CC 50 8A DD F1 05 36 86
//0x12F  37 7C 8A DD D4 05 33 06
//0x12F  8C 50 8A DD F4 05 30 06
//0x12F  50 54 8A DD  F1 05 30 C1
//0x12F  B7 5A 8A DD  F4 05 30 01
//+      CC 50 8A DD F1 05 36 86
//      00  00 8A DD F1 15 30 02  
  can12f_off.can_id  = 0x12f;
  can12f_off.can_dlc = 8;
  can12f_off.data[0] = 0x00;
  can12f_off.data[1] = 0x00;
  can12f_off.data[2] = 0x00;
  can12f_off.data[3] = 0x00;
  can12f_off.data[4] = 0x00;
  can12f_off.data[5] = 0x00;
  can12f_off.data[6] = 0x00;
  can12f_off.data[7] = 0x00;


  // ignition on
  can130.can_id  = 0x130;
  can130.can_dlc = 6;
  can130.data[0] = 0x45;
  can130.data[1] = 0x42;
  can130.data[2] = 0x69;
  can130.data[3] = 0x8f;
  can130.data[4] = 0xe2;
  can130.data[4] = 0xfe;

  // speed
  can1a1.can_id  = 0x1a1;
  can1a1.can_dlc = 5;
  can1a1.data[0] = 0xb3;//be;//b3
  can1a1.data[1] = 0xcb;//de;//cb
  can1a1.data[2] = 0x00;//12;//00
  can1a1.data[3] = 0x00;//04;//00
  can1a1.data[4] = 0x81;//91;//81


  // PDC distance
  can1c2.can_id  = 0x1c2;
  can1c2.can_dlc = 8;
  can1c2.data[0] = 0x3a;
  can1c2.data[1] = 0xfe;
  can1c2.data[2] = 0xfe;
  can1c2.data[3] = 0xfe;
  can1c2.data[4] = 0x15;
  can1c2.data[5] = 0x93;
  can1c2.data[6] = 0xfe;
  can1c2.data[7] = 0xfe;      

  // PDC
  can4e4.can_id  = 0x4e4;
  can4e4.can_dlc = 8;
  can4e4.data[0] = 0x65;
  can4e4.data[1] = 0x42;
  can4e4.data[2] = 0xff;
  can4e4.data[3] = 0x01;
  can4e4.data[4] = 0xff;
  can4e4.data[5] = 0xff;
  can4e4.data[6] = 0xff;
  can4e4.data[7] = 0xff;      

  // PDC acoustic
  can1c6.can_id  = 0x1c6;
  can1c6.can_dlc = 4;
  can1c6.data[0] = 0x00;//52 close / 32 - far
  can1c6.data[1] = 0x00;
  can1c6.data[2] = 0x52;
  can1c6.data[3] = 0x00;

  can37a.can_id = 0x37a;
  can37a.can_dlc = 8;
  can37a.data[0] = 0x0e;
  can37a.data[1] = 0x59;
  can37a.data[2] = 0x49;
  can37a.data[3] = 0x09;
  can37a.data[4] = 0xff;
  can37a.data[5] = 0xff;  
  can37a.data[6] = 0x1f;
  can37a.data[7] = 0xff;  

  // brightness
  can202.can_id  = 0x202;
  can202.can_dlc = 2;
  can202.data[0] = 0xfe;//brightness FE - off
  can202.data[1] = 0xff;

  // iDrive lag remove
  can510.can_id  = 0x510;
  can510.can_dlc = 8;
  can510.data[0] = 0x00;
  can510.data[1] = 0x00;
  can510.data[2] = 0x00;
  can510.data[3] = 0x00;
  can510.data[4] = 0xff;
  can510.data[5] = 0xff;
  can510.data[6] = 0x00;
  can510.data[7] = 0x10;

  //lang
  can2f7.can_id = 0x2f7;
  can2f7.can_dlc = 6;
  can2f7.data[0] = 0x0e;
  can2f7.data[1] = 0x59;
  can2f7.data[2] = 0x49;
  can2f7.data[3] = 0x09;
  can2f7.data[4] = 0x11;
  can2f7.data[5] = 0xf1;

//  can380.can_id = 0x380;
//  can380.can_dlc = 7;
//  can380.data[0] = 0x35;
//  can380.data[1] = 0x4b;
//  can380.data[2] = 0x35;
//  can380.data[3] = 0x37;
//  can380.data[4] = 0x32;
//  can380.data[5] = 0x31;
//  can380.data[6] = 0x38;   

  //0x2A0; 8; 88 88 80 0 16 40 86 0 - trunk
  can2a0.can_id = 0x2a0;
  can2a0.can_dlc = 8;
  can2a0.data[0] = 0x88;
  can2a0.data[1] = 0x88;
  can2a0.data[2] = 0x80;
  can2a0.data[3] = 0x00;
  can2a0.data[4] = 0x16;
  can2a0.data[5] = 0x40;
  can2a0.data[6] = 0x86;
  can2a0.data[7] = 0x00;

  //  Alarm indicator
  //  1F6 B1 F2
  //  1F6 B1 F1
  //  1F6 80 F0
  can1f6.can_id = 0x1f6;
  can1f6.can_dlc = 2;
  can1f6.data[0] = 0xb1;
  can1f6.data[1] = 0xf2;
  
  //0x6F1 72 06 30 29 07 01 01 04 -  зажигает все поворотники на 700мc
  //0x6F1 72 06 30 03 07 0A 00 64 - задний левый вкл
  //0x6F1 72 06 30 03 07 0B 00 64 - задний правый вкл
  //0x6F1 72 06 30 03 07 0A 00 00 - задний левый вЫкл
  //0x6F1 72 06 30 03 07 0B 00 00 - задний правый вЫкл
  can6f1.can_id = 0x6f1;
  can6f1.can_dlc = 8;
  can6f1.data[0] = 0x72;
  can6f1.data[1] = 0x06;
  can6f1.data[2] = 0x30;
  can6f1.data[3] = 0x29;
  can6f1.data[4] = 0x07;
  can6f1.data[5] = 0x01;
  can6f1.data[6] = 0x01;
  can6f1.data[7] = 0x04;
}

void power_off() {
    enableBacklight(false);
    mcpNBT.sendMessage(&can202);//turn off
    mcpNBT.sendMessage(&can12f_off);  
}

bool reset_idle(bool activate = false) {
  if (activate) {
    idle_timer = millis();
    idle = true;      
    return false;
  }
  
  int idle_timeout = 10000;  
  if (idle) {
    if (millis() - idle_timer > idle_timeout) {
      idle_timer = 0;
      idle = false;
      return true;
    }      
  }
  return false;
}

void modePressed(void)
{
  int btns_timeout = 300;
  if (millis() - btns_timer > btns_timeout) {
    btns_timer = millis();

    canPanel.can_id  = 0x0a1;
    canPanel.can_dlc = 2;
    canPanel.data[0] = 0x10;
    canPanel.data[1] = 0xff;
  
    mcpNBT.sendMessage(&canPanel);

    // zero delay  
    canPanel.can_id  = 0x0a1;
    canPanel.can_dlc = 2;
    canPanel.data[0] = 0x00;
    canPanel.data[1] = 0xff;
  
    mcpNBT.sendMessage(&canPanel);
  }
}

void fmamPressed(void)
{

  int btns_timeout = 300;
  if (millis() - btns_timer > btns_timeout) {
    btns_timer = millis();
    canPanel.can_id  = 0x0a1;
    canPanel.can_dlc = 2;
    canPanel.data[0] = 0x40;
    canPanel.data[1] = 0xff;
    
    mcpNBT.sendMessage(&canPanel);

    // zero delay  
    canPanel.can_id  = 0x0a1;
    canPanel.can_dlc = 2;
    canPanel.data[0] = 0x00;
    canPanel.data[1] = 0xff;
  
    mcpNBT.sendMessage(&canPanel);
  }
}

void mutePressed(void)
{
  int btns_timeout = 300;
  if (millis() - btns_timer > btns_timeout) {
    btns_timer = millis();
    canPanel.can_id  = 0x0a1;
    canPanel.can_dlc = 2;
    canPanel.data[0] = 0x04;
    canPanel.data[1] = 0xff;
  
    mcpNBT.sendMessage(&canPanel);

    // zero delay  
    canPanel.can_id  = 0x0a1;
    canPanel.can_dlc = 2;
    canPanel.data[0] = 0x00;
    canPanel.data[1] = 0xff;
  
    mcpNBT.sendMessage(&canPanel);
  }
}

void ejectPressed(void)
{

  int btns_timeout = 300;
  if (millis() - btns_timer > btns_timeout) {
    btns_timer = millis();
    
    canPanel.can_id  = 0x0a1;
    canPanel.can_dlc = 2;
    canPanel.data[0] = 0x01;
    canPanel.data[1] = 0xff;
  
    mcpNBT.sendMessage(&canPanel);

    // zero delay  
    canPanel.can_id  = 0x0a1;
    canPanel.can_dlc = 2;
    canPanel.data[0] = 0x00;
    canPanel.data[1] = 0xff;
  
    mcpNBT.sendMessage(&canPanel);
  }
}

void favPressed(uint8_t *addr1, uint8_t *addr2) 
{
  canPanel.can_id  = 0x0a2;
  canPanel.can_dlc = 2;
  canPanel.data[0] = addr1;
  canPanel.data[1] = addr2;

  mcpNBT.sendMessage(&canPanel);
}

void volChange(uint8_t *vol, bool up=true) 
{
  canPanel.can_id  = 0x0f1;//0x0f1
  canPanel.can_dlc = 2;
  canPanel.data[0] = vol;
  canPanel.data[1] = up ? 0xfd : 0xfe;

  mcpNBT.sendMessage(&canPanel);
}

void backNextPressed(bool next=true)
{
  canPanel.can_id  = 0x0a3;
  canPanel.can_dlc = 2;
  canPanel.data[0] = next ? 0xfd : 0xfe;
  canPanel.data[1] = 0xff;
  
  mcpNBT.sendMessage(&canPanel);
  delay(200);
  canPanel.data[0] = 0x00;
  mcpNBT.sendMessage(&canPanel);

  //  int btns_timeout = 300;
  //  if (millis() - btns_timer > btns_timeout) {
  //    btns_timer = millis();      
  //    mcpNBT.sendMessage(&canPanel);
  //  }
}


void pollingKeys(void)
{
  uint8_t linMessage[8];
  lin.recv(0x03, linMessage, 8, 0);

  //@todo avoid doble pressing.
  //      if command sent to CAN, repeated command should be sent after X ms timeout only

  if(linMessage[2]==0x00 && linMessage[3]==0x80) {
      favPressed(linMessage[0], linMessage[1]);
      
  } else if(linMessage[2]==0x01 && linMessage[3]==0x80) {
      modePressed();
      
  }else if(linMessage[2]==0x04 && linMessage[3]==0x80) {
      fmamPressed();
      
  }else if(linMessage[2]==0x00 && linMessage[3]==0x84) {
      mutePressed();
      
  }else if(linMessage[2]==0x10 && linMessage[3]==0x80) {
      ejectPressed();
      
  }else if(linMessage[2]==0x00 && linMessage[3]==0x81) {
      volChange(linMessage[4], true);
      
  }else if(linMessage[2]==0x00 && linMessage[3]==0x82) {
      volChange(linMessage[4], false);
      
  }else if(linMessage[2]==0x40 && linMessage[3]==0x80) {
      backNextPressed(true);
      
  }else if(linMessage[2]==0x80 && linMessage[3]==0x80) {
      backNextPressed(false);   
  }
}

void alarm() {

  mcpNBT.sendMessage(&can1c2);
  mcpNBT.sendMessage(&can4e4);
  mcpNBT.sendMessage(&can1c6);
//  mcpNBT.sendMessage(&can37a);
  delay(100);

  //0x6F1 72 06 30 29 07 01 01 04 -  зажигает все поворотники на 700мc
  //0x6F1 72 06 30 03 07 0A 00 64 - задний левый вкл
  //0x6F1 72 06 30 03 07 0B 00 64 - задний правый вкл
  //0x6F1 72 06 30 03 07 0A 00 00 - задний левый вЫкл
  //0x6F1 72 06 30 03 07 0B 00 00 - задний правый вЫкл
  //  can6f1.data[0] = 0x72;
  //  can6f1.data[1] = 0x06;
  //  can6f1.data[2] = 0x30;
  //  can6f1.data[3] = 0x29;//03
  //  can6f1.data[4] = 0x07;
  //  can6f1.data[5] = 0x01;//0a
  //  can6f1.data[6] = 0x01;//00
  //  can6f1.data[7] = 0x04;//64
  //  mcpFEM.sendMessage(&can6f1);
  //  delay(1000);
  //  can6f1.data[7] = 0x00;
  //  mcpFEM.sendMessage(&can6f1);
  //  delay(1000);

//  can1f6.data[0] = 0xb1;
//  can1f6.data[1] = 0xf2;
//  mcpFEM.sendMessage(&can1f6);
//  
//  can1f6.data[0] = 0xb1;
//  can1f6.data[1] = 0xf1;
//  mcpFEM.sendMessage(&can1f6);
//  
//  delay(700);
//
//  mcpFEM.sendMessage(&can6f1);          
//    
//  can1f6.data[0] = 0xb1;
//  can1f6.data[1] = 0xf2;
//  mcpFEM.sendMessage(&can1f6);
//  
//  can1f6.data[0] = 0xb1;
//  can1f6.data[1] = 0xf1;
//  mcpFEM.sendMessage(&can1f6);
//  
//  delay(700);
//  
//  can1f6.data[0] = 0x80;
//  can1f6.data[1] = 0xf0;
//  mcpFEM.sendMessage(&can1f6);
    
}

void forward_to_nbt() {
  // From Car to NBT
  if (mcpFEM.readMessage(&canMsg) == MCP2515::ERROR_OK) {

    switch (canMsg.can_id) {

      //case 0x1D6:
      //  //voice c0 0d
      //  if (canMsg.data[0] == 0xc0 && canMsg.data[1] == 0x0d) {
      //    alarm();
      //    return;
      //  }
      //
      //  if (canMsg.data[0] == 0xc0 && canMsg.data[1] == 0x4c) {
      //    canMsg.data[1] = 0x1c;
      //  }
      //  break;
                
      case 0x202:

        if (canMsg.data[0] == 0xfe) {
          //off
          audioPanelBacklight(0x00);
        } else if(canMsg.data[0] == 0xff) {
          //max, n/a
          audioPanelBacklight(0xfe);
        } else {                    
          audioPanelBacklight(canMsg.data[0]);
        }        
        break;
                
      case 0x380://VIN        
        return;
        break;
          
      case 0x130:
        //forward and also send 12f
        uint8_t ign_status = canMsg.data[0];
        if (ign_status != 0x00) {
          reset_idle(true);
        } else {
          if (idle) {
            power_off();  
          }
          idle = false;  
        }
        break;
          
      case 0x12f: //not possible at e-series
        return;
        break;

      case 0x2b4:
        //00 F2 - closed or 00 F1 - opened
        return;
        break;

      case 0x2f7: //overwrite lang and format
        return;
        break;

      //case 0x1a1: //video in motion
      //  return;
      //  break;
          
//      case 0x1b4://convert e-series speed to f-seriew speed
//        uint16_t s = ((canMsg.data[1]-0xd0)*256 + canMsg.data[0]) / 16 * 100;
//        can1a1.data[3] = s >> 8;
//        can1a1.data[2] = s & 0xff;
//        mcpNBT.sendMessage(&can1a1);
//        return;
//        break;
    }
    
    //12f;//nbt wake-up
    //130;//ign on
    //1a1;//speed ?
    //1c2;//PDC ?!!
    //202;//dimm
    //2f7;//lang
    //510;//idrive lag
    
    if (idle || (canMsg.can_id == 0x202)) {
      mcpNBT.sendMessage(&canMsg);
    }
  }
}

void forward_from_nbt() {
  // From NBT to Car
  if (mcpNBT.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    switch(canMsg.can_id) {
       case 0x24F:
       case 0x348:
       case 0x34A:
       case 0x34C:
       case 0x34E:
       case 0x380://VIN
       case 0x38D:
       case 0x413:
       case 0x415:
       case 0x43D:
       case 0x43C:
       case 0x563:
       case 0x567:
       case 0x5E7:
       case 0x667://-- idle idrive
         return;
         break;
      case 0x264://-rotate
      case 0x267://-click
      case 0xBF://touch
        return;
        break;      
    }
    mcpFEM.sendMessage(&canMsg);
  }
}

void loop(void)
{

  each_time();
    
  int refresh20_timeout = 20;
  if (millis() - refresh20_timer > refresh20_timeout) {
    refresh20_timer = millis();
    each_20ms();
  }

  int refresh100_timeout = 100;
  if (millis() - refresh100_timer > refresh100_timeout) {
    refresh100_timer = millis();
    each_100ms();
  }

  //  int refresh500_timeout = 500;
  //  if (millis() - refresh500_timer > refresh500_timeout) {
  //    refresh500_timer = millis();
  //    each_500ms();
  //  }

  int refresh1000_timeout = 1000;
  if (millis() - refresh1000_timer > refresh1000_timeout) {
    refresh1000_timer = millis();
    each_1000ms();
  }
}

void each_time() {
  forward_to_nbt();
  if (idle) {
    forward_from_nbt();
  }
}

void each_20ms() {
  if (idle) {
    pollingKeys();
  }
}

void each_100ms() {
  if (idle) { //send if 130 received
    mcpNBT.sendMessage(&can12f); //NBT wake-up
    mcpNBT.sendMessage(&can510);//wakes up idrive    
    
    mcpNBT.sendMessage(&can1a1); // vim
  }

  //  mcpNBT.sendMessage(&can130); //deactivated for bridge
  //  mcpNBT.sendMessage(&can1a0);
  
  //  mcpNBT.sendMessage(&can21a);
  //  mcpNBT.sendMessage(&can273);
  //  mcpNBT.sendMessage(&can575);
  //  mcpNBT.sendMessage(&canTmp);
}

void each_500ms() {
  //  mcpNBT.sendMessage(&can2f4); 
  //  mcpNBT.sendMessage(&can380); 
}

void each_1000ms() {

  //  mcpNBT.sendMessage(&can3be);
  //  mcpNBT.sendMessage(&can3f9);
  
  if (idle) {  
    mcpNBT.sendMessage(&can2f7);
  }
    
  bool deactivated;
  deactivated = reset_idle();

  if (deactivated) {
    power_off();
  }
}
