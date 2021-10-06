/*************************************************************************************
Read and globally save I²C SHT21 values
/*************************************************************************************/
void updateSHT21()
{
  gTempFl = sht21.readTemperature();
  gTempStr = String(gTempFl, 1);

  gHumidityFl = sht21.readHumidity();  
  gHumidityStr = String(gHumidityFl, 0);
}
