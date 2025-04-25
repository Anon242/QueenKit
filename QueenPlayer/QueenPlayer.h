#include <SoftwareSerial.h>

class QueenPlayer{
    public:

    QueenPlayer(){
        #if defined _R400v2
            playerSerial = new SoftwareSerial(3, 4); // 3 - 4
        #elif defined _R400v3
            playerSerial = new SoftwareSerial(3, 4); // 3 - 4
        #elif defined _R404v4
            playerSerial = new SoftwareSerial(3, 4); // 3 - 4
        #elif defined _R404v5
            playerSerial = new SoftwareSerial(3, 4); // 3 - 4
        #elif defined _R408v2
            playerSerial = new SoftwareSerial(3, 4); // 3 - 4
        #elif defined _R408v3
            playerSerial = new SoftwareSerial(3, 4); // 3 - 4
        #elif defined _R416
            playerSerial = new SoftwareSerial(3, 4); // 3 - 4
        #elif defined _QueenUnisense4
            // Serial 2
        #elif defined _QueenUnisense3v1
            playerSerial = new SoftwareSerial(40, 41);
        #else 
            #error Плата не поддерживается.
        #endif
    }

    void init(){
        playerBegin();
    }

    void play(uint8_t track, uint8_t volume){
        if (player_track != track) {
          player_track = track;
          if (player_track > 0)
            player(0x03, player_track);
          else if (player_track == 0)
            player(0x0E, 0x00);
        }
        if (player_volume != volume) {
          player_volume = volume;
          player(0x06, player_volume < 30 ? player_volume : 30);
        }
    }

    private:
    SoftwareSerial* playerSerial; 
    uint8_t player_track = 0;
    uint8_t player_volume = 0;

    
    void playerBegin(){
        #if defined _QueenUnisense4
            Serial2.begin(9600);
        #else
            playerSerial->begin(9600); // init player
        #endif
    }

    uint8_t player_buffer[10] = {
        0x7E, // [0] start byte, always 0x7E
        0xFF, // [1] version, always 0xFF
        0x06, // [2] length, always 0x06
        0x00, // [3]*command: 0x03 - track, 0x06 - volume, 0x0E - pause
        0x00, // [4] feedback, lways 0x00
        0x00, // [5] high byte, always 0x00
        0x00, // [6]*low byte (parameter) track or volume
        0xFE, // [7] ???
        0x00, // [8]*checksumm
        0xEF  // [9] end byte, always 0xEF
    };

    void player( uint8_t command, uint8_t value )
{
  player_buffer[ 3 ] = command;
  player_buffer[ 6 ] = value;
  uint8_t checksum = 0;
  for ( int i = 2; i < 8; i ++ ) checksum += player_buffer[i];
  player_buffer[8] = (uint8_t) ~checksum;
  
    #if defined _QueenUnisense4
        Serial2.write(player_buffer, 10);
    #else
        playerSerial->write( player_buffer, 10 );
    #endif
} 
};       
