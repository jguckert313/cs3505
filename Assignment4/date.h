
#ifndef DATE_H
#define DATE_H

namespace assignment4
{

  // maybe make date one number for days and one for a year or maybe just days
  class date
  {
  public:
    date(int month, int day, int year);
    date(date & other);
    ~date();
    
    //int get_month();
    //int get_day();
    //int get_year();

    

  private:
    int month;
    int day;
    int year; 
  };
}

#endif
