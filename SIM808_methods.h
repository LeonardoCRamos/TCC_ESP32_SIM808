#ifndef SIM808_H
#define SIM808_H

#if (ARDUINO >= 100)
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

class SIM808_methods {
  public:
    SIM808_methods(HardwareSerial *mySerial);
    bool init(void);
    bool SIM_status(void);
    bool turnon_GPS(void);
    bool turnoff_GPS(void);
    bool status_GPS(void);
    void read_GPS(void);
    bool deleteall_SMS(void);
    bool delete_SMS(char* index);
    int new_SMS(void);
    bool read_SMS_data(int index, char* UUID, int UUID_size);
    int read_SMS_safe(int index, int phone_size, char* PHONE);
    bool send_SMS(char* PHONE, char* MESSAGE);

    struct userdata{
      char user_phone[20];
      int user_phone_size;
      char help_phone[20];
      float lat_home;
      float lon_home;
      float safe_radius;
    }USERdata;

    struct gspdata{
      int year;
      int month;
      int day;
      int hour;
      int minute;
      int second;
      float lat;
      float lon;
      float speed_kph;
      float heading;      
    }GPSdata;

  private:
    void reset_buffer(void);
    char* read_buffer(char* msg_sent, int msg_size);
    bool wait_response(char r);
    void ERROR_code(char x);
};


#endif
