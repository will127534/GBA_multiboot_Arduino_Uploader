#include "SPI.h"

uint32_t crc = 0xc387;
uint32_t hh,rr,encryptionseed;
uint32_t gba_file_length = 0;
void setup() {
  // put your setup code here, to run once:
Serial.begin(115200);
pinMode(MISO,HIGH);
pinMode(MOSI,HIGH);
pinMode(SCK,HIGH);
pinMode(MISO,OUTPUT);
pinMode(MOSI,OUTPUT);
pinMode(SCK,OUTPUT);

while(!Serial);
SPI.begin();
SPI.setDataMode(SPI_MODE3);
SPI.setClockDivider(SPI_CLOCK_DIV64);
pinMode(13,OUTPUT);
digitalWrite(13,HIGH);
 
while(1){
  if(Serial.read()=='C'){
    gba_file_length =Serial.parseInt();
    gba_file_length = (gba_file_length+0xf)&(~0xf);
    Serial.println(gba_file_length);
    break;
  }
}


  uint32_t read_from_gba = 0;
  read_from_gba = GBA_transfer(0x00006202);
  while(read_from_gba!=0x72026202)
  {
    delay(100);
    read_from_gba = GBA_transfer(0x00006202);
    Serial.print("Looking for GBA\r\n");        
  }
        
    read_from_gba = GBA_transfer(0x00006202);
    Serial.print("Found GBA\r\n");

    read_from_gba = GBA_transfer(0x00006102);
    Serial.print("Recognition ok\r\n");
    
    Serial.print("S\r\n");
    uint32_t i;
    for(int Counter=0;Counter<=0x5f;Counter++)
    {
       while(Serial.available()<2){
       }
       byte a = Serial.read();
       byte b = Serial.read();
        i =(uint32_t)b<<8 | a;         //Read in 16 bits of .gba file
        //Serial.println(i,HEX);
         Serial.println("A");
        read_from_gba = GBA_transfer(0x0000ffff&i);
       
    }

    read_from_gba = GBA_transfer(0x00006200);
    Serial.print("Transfer of header data complete\r\n");
 
    read_from_gba = GBA_transfer(0x00006202);
    Serial.print("Exchange master/slave info again\r\n");
    
    initCryption();
    Serial.println("S2");
    for(uint32_t gba_file_location = 0xc0; gba_file_location<gba_file_length;gba_file_location+=4)
    {    
      while(Serial.available()<4){
      }
        byte a = Serial.read();
        byte b = Serial.read();
        byte c = Serial.read();
        byte d = Serial.read();
        uint32_t gba_data = a;               //Read in 32 bits of .gba file
        gba_data = (uint32_t)b<<8|gba_data;               
        gba_data = (uint32_t)c<<16|gba_data;  
              
        gba_data = (uint32_t)d<<24|gba_data;  

        uint32_t gba_data2 =encryption(gba_data,gba_file_location);
        //Serial.println("A");
        Serial.print("ptr:");Serial.print(gba_file_location,HEX);Serial.print(" Data:");Serial.println(gba_data,HEX);
        crc = calcCRC(gba_data,crc);
        GBA_transfer(gba_data2);
        
         
     }
 
   int retry = 0;
    while (GBA_transfer(0x00000065)!=0x00750065)
    {
       Serial.print("Wait for GBA to respond with CRC\n\r");
       retry ++;
       if(retry>10){
          break;
       }
    }
    
    Serial.print("GBA ready with CRC\n\r");
    
    read_from_gba = GBA_transfer(0x00000066);//EOF Signal
    
    Serial.print("Let's exchange CRC!\n\r");
//    uint32_t dataSend = ((((read_from_gba>>16)&0xFF00 + FinalKeyHighByte)<<8) | 0x0FFFF0000) + FinalKeyLowByte;
    uint32_t dataSend = finalCRC(read_from_gba);    

    //crc = calcCRC(dataSend,crc);
    read_from_gba = GBA_transfer(dataSend);


    Serial.print("CRC is:");Serial.print(crc,HEX);Serial.print(" Actual:");Serial.println(read_from_gba,HEX);

    
SPI.end();
pinMode(SS,INPUT);
pinMode(MISO,INPUT);
pinMode(MOSI,INPUT);
pinMode(SCK,INPUT);

}
uint32_t GBA_transfer(uint32_t data){
  byte buf[4];
  uint32_t ret = 0;
  buf[0] = data>>24;
  buf[1] = data>>16;
  buf[2] = data>>8;
  buf[3] = data;
  
  digitalWrite(13,LOW);
  SPI.transfer(buf, 4);
  digitalWrite(13,HIGH);
  for(int i=0;i<4;i++){
    ret |= (uint32_t)buf[i]<<((3-i)*8);
  }
  
  Serial.print(">>>");Serial.println(data,HEX);
  Serial.print("<<<");Serial.println(ret,HEX);
  return ret;
}
uint32_t finalCRC(uint32_t data){
    uint32_t crctemp = ((((data&0xFF00)+rr)<<8)|0xFFFF0000)+hh;
    crc = calcCRC(crctemp,crc);

    return crc;
}
uint32_t calcCRC(uint32_t data,uint32_t crc)
{
    int bit;
    for (bit = 0 ; bit < 32 ; bit++)
    {
        uint32_t tmp = crc ^ data;
        crc >>= 1;
        data >>= 1;
        if (tmp & 0x01) crc ^= 0xc37b;
    }
    return crc;
}
uint32_t encryption(uint32_t data,uint32_t ptr){
    ptr = ~(ptr+0x02000000)+1;
    encryptionseed = (encryptionseed * 0x6F646573)+1;

    data = (encryptionseed ^ data)^(ptr ^ 0x43202F2F);
    return data;
}
void initCryption(){
    uint32_t read_from_gba = 0;
    read_from_gba = GBA_transfer(0x000063d1);  
    Serial.print("Send encryption key\r\n");

    read_from_gba = GBA_transfer(0x000063d1);  
    Serial.print("Encryption key received 0x73hh****\r\n");

    uint32_t data = read_from_gba>>16;
    encryptionseed = ((data&0x000000FF)<<8)|0x0FFFF00d1;
    hh = ((data&0x000000FF)+0x0000020F) & 0x000000FF;

    Serial.print("Seed:");Serial.print(encryptionseed,HEX);
    Serial.print(" hh:");Serial.println(hh,HEX);

    data = 0x6400 | hh;
    GBA_transfer(data);
    Serial.println("Encryption confirmation");


    data = ((gba_file_length-0xC0)>>2)-0x34;
    read_from_gba = GBA_transfer(data);
    Serial.println("Length information exchange");

    rr = (read_from_gba >> 16) & 0xff;


    
}

void loop() {
  // put your main code here, to run repeatedly:
  /*
if(Serial.available()>=4){
  byte buf[4];
  Serial.readBytes(buf,4);
  */


}
