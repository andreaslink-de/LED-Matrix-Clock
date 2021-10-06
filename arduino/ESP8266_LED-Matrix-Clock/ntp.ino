#ifdef USE_NTP_TIME

unsigned long gDeltaUnixTimestamp = 0;


unsigned long getNowUnixTimestamp()
{
  return timeClient.getEpochTime();
}


int getNowWeekday() //0 is Sunday
{
  return timeClient.getDay();
}

int getNowHours()
{
  return timeClient.getHours();
}

int getNowMinutes()
{
  return timeClient.getMinutes();
}

int getNowSeconds()
{
  return timeClient.getSeconds();
}

String getNowFormatedTime()  //21:42:23
{
  return timeClient.getFormattedTime();
}


String getNowSimpleFormatedTime()  //21:42
{
  if (timeClient.getMinutes() < 10)
  {
    return (String)timeClient.getHours() + ":0" + (String)timeClient.getMinutes();
  } else {
    return (String)timeClient.getHours() + ":" + (String)timeClient.getMinutes();
  }
}


long calculateDeltaSeconds(unsigned long pTargetUnixTimestamp)
{
  //Always takes now as reference
  long delta = pTargetUnixTimestamp - getNowUnixTimestamp();

  if (delta <= 0)
  {
    //Date is in the past, so force to 0
    delta = 0;
  }
  
  return delta;
}


boolean timeToSleep(const DateTime &dt)
{
  return (dt.hour() >= 1 && dt.hour() < 6);
  //return (dt.hour() >= 22 && dt.minute() == 41);
}


void setSummertime(boolean pSummer)
{
  if (pSummer)
  {
    timeClient.setTimeOffset(7200);
    //DEBUGln("Summertime (+2h)");
  } else {
    timeClient.setTimeOffset(3600);
    //DEBUGln("Wintertime (+1h)");
  }
}


void doWeHaveSummerOrWinterTime()
{
  /*
   * The EU of course has a guideline on it that determines the daylight saving time to start on the last Sunday 
   * of March and to end on the last Sunday of October. In the EU the hour to switch is at 01:00 UTC. In practice 
   * UTC is equal to GMT (Greenwich Mean Time).
  */

  DateTime rightNow(getNowUnixTimestamp());
  
  int dow = getNowWeekday();
  int mon = rightNow.month();
  int d = rightNow.day();
  int h = rightNow.hour();

  // Debug:
  //dow = 3;
  //mon = 10;
  //d = 28;
  //h = 14;

  // As October has 31 days, we know that the last Sunday (dow=0) will always fall from the 25th to the 31st.
  // Outside this time it is for sure wintertime.
  if (mon >= 11 || mon < 3) 
  {
    setSummertime(false);
    return;
  } else if (mon == 10 && d < 25)
  {
    setSummertime(true);
    return;
  } else if (dow == 0 && mon == 10 && d >= 25 && h >= 3)
  {
    setSummertime(false);
    return;
  } else if (mon == 10 && d > 25)
  {
    if (dow == 0 && h >= 3)
    {
      setSummertime(false);
      return;
    } else if (dow > 0) {
      switch (d)
      {
        case 26:
          {
            if (dow -1 == 0)
            {
              //The day before was a Sunday
              setSummertime(false);
              return;
            }        
          }
          break;

        case 27:
          {
            for (int i=1; i<=2; i++)
            {
              if (dow -i == 0)
              {
                //Before there was a Sunday
                setSummertime(false);
                return;
              } 
            }
          }
          break;

        case 28:
          {
            for (int i=1; i<=3; i++)
            {
              if (dow -i == 0)
              {
                //Before there was a Sunday
                setSummertime(false);
                return;
              } 
            }
          }
          break;

        case 29:
          {
            for (int i=1; i<=4; i++)
            {
              if (dow -i == 0)
              {
                //Before there was a Sunday
                setSummertime(false);
                return;
              } 
            }
          }
          break;

        
        case 30:
          {
            for (int i=1; i<=5; i++)
            {
              if (dow -i == 0)
              {
                //Before there was a Sunday
                setSummertime(false);
                return;
              } 
            }
          }
          break;

        
        case 31:
          {
            for (int i=1; i<=6; i++)
            {
              if (dow -i == 0)
              {
                //Before there was a Sunday
                setSummertime(false);
                return;
              } 
            }
          }
          break;
      }        
    }
  }


  /*
   * Anyway lets do a check if we have the right routine: Suppose October 1 is a Sunday. That means the 25th is a 
   * Wednesday and the 29th a Sunday (the last and 5th Sunday), so the routine will indeed be activated on the 29th 
   * as all conditions are met. Suppose October starts on a Monday. That means there are only 4 Sundays and the last 
   * Sunday will fall on the 28th. Again, that is between 25 and 31 so the condition is met. Suppose October first 
   * falls on a Thursday, then the 25th will be a Sunday (the fourth Sunday) and the 31st will be a Saturday, so 
   * again the condition will be met for the last (and in this case 4th Sunday).
   * 
   * To start summertime/daylightsaving time on the last Sunday in March is mutatis mutandis the same.
   * March also has 31 days so the calculations are the same. This time though we check at 2 am and a 
   * reset DST flag and then set the clock to 3 am.
  */

  if (mon >= 4 && mon < 10)
  {
    setSummertime(true);
    return;
  } else if (mon == 3 && d < 25)
  {
    setSummertime(false);
    return;    
  } else if (dow == 0 && mon == 3 && d >= 25 && h >= 2)
  {
    setSummertime(true);
    return;
  } else if (mon == 3 && d > 25)
  {
    if (dow == 0 && h >= 2)
    {
      setSummertime(true);
      return;
    } else if (dow > 0) {
      switch (d)
      {
        case 26:
          {
            if (dow -1 == 0)
            {
              //The day before was a Sunday
              setSummertime(true);
              return;
            }        
          }
          break;

        case 27:
          {
            for (int i=1; i<=2; i++)
            {
              if (dow -i == 0)
              {
                //Before there was a Sunday
                setSummertime(true);
                return;
              } 
            }
          }
          break;

        case 28:
          {
            for (int i=1; i<=3; i++)
            {
              if (dow -i == 0)
              {
                //Before there was a Sunday
                setSummertime(true);
                return;
              } 
            }
          }
          break;

        case 29:
          {
            for (int i=1; i<=4; i++)
            {
              if (dow -i == 0)
              {
                //Before there was a Sunday
                setSummertime(true);
                return;
              } 
            }
          }
          break;

        
        case 30:
          {
            for (int i=1; i<=5; i++)
            {
              if (dow -i == 0)
              {
                //Before there was a Sunday
                setSummertime(true);
                return;
              } 
            }
          }
          break;

        
        case 31:
          {
            for (int i=1; i<=6; i++)
            {
              if (dow -i == 0)
              {
                //Before there was a Sunday
                setSummertime(true);
                return;
              } 
            }
          }
          break;
      }        
    }
  }

  //Fallback is always wintertime
  setSummertime(false);  
}


/*
void updateGlobalClock()
{  
  unsigned long unixNow = getNowUnixTimestamp();
  DateTime rightNow(unixNow);
  
  //Debug
  printDateOnSerial("Current DateTime: ", rightNow);

  //Set clock globally
  gClock = (rightNow.hour() < 10) ? "0" + (String)rightNow.hour() + ":" : (String)rightNow.hour() + ":";
  gClock += (rightNow.minute() < 10) ? "0" + (String)rightNow.minute() + ":" : (String)rightNow.minute() + ":";      
  gClock += (rightNow.second() < 10) ? "0" + (String)rightNow.second() : (String)rightNow.second();

  DEBUGln(gClock);
}
*/


void printDateOnSerial(const char* txt, const DateTime& dt) 
{  
  DEBUG(txt);
  DEBUG(' ');
  Serial.print(dt.year(), DEC);  
  
  if (dt.month() < 10) DEBUG("-0") else DEBUG('-');
  Serial.print(dt.month(), DEC);  
  
  if (dt.day() < 10) DEBUG("-0") else DEBUG('-');
  Serial.print(dt.day(), DEC);  
  
  DEBUG(' ');  
  Serial.print(dt.hour(), DEC);
  
  if (dt.minute() < 10) DEBUG(":0") else DEBUG(':');  
  Serial.print(dt.minute(), DEC);
    
  if (dt.second() < 10) DEBUG(":0") else DEBUG(':');  
  Serial.print(dt.second(), DEC);

  DEBUG(" = ");
  DEBUG(dt.unixtime());
  DEBUG("s / ");
  DEBUG(dt.unixtime() / 86400L);
  DEBUG("d since 1970");

  DEBUGln();
}

void printTimeSpanOnSerial(const char* txt, const TimeSpan& ts) 
{
  DEBUG(txt);
  DEBUG(" ");
  Serial.print(ts.days(), DEC);
  DEBUG(" days ");
  Serial.print(ts.hours(), DEC);
  DEBUG(" hours ");
  Serial.print(ts.minutes(), DEC);
  DEBUG(" minutes ");
  Serial.print(ts.seconds(), DEC);
  DEBUG(" seconds (");
  Serial.print(ts.totalseconds(), DEC);
  DEBUG(" total seconds)");
  DEBUGln();
}
  
#endif
