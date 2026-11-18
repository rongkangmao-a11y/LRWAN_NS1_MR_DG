/**
  ******************************************************************************
  * @file    master_app.c
  * @author  MCD Application Team
  * @brief   Application of the AT Master
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "platform.h"
#include "sys_app.h"
#include "master_app.h"
#include "lora_driver.h"
#include "sys_sensors.h"
#include "rtc_if.h"
#include "stdio.h"


#include "lrwan_ns1_printf.h"

#include ATCMD_MODEM        /* preprocessing definition in sys_conf.h*/



/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/


/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* CAYENNE_LLP is myDevices Application server*/
#define CAYENNE_LPP
#define LPP_DATATYPE_DIGITAL_INPUT 0x0
#define LPP_DATATYPE_DIGITAL_INPUT_SIZE 1
#define LPP_DATATYPE_HUMIDITY 0x68
#define LPP_DATATYPE_HUMIDITY_SIZE 1        // humidity sensor data size
#define LPP_DATATYPE_TEMPERATURE 0x67
#define LPP_DATATYPE_TEMPERATURE_SIZE 2
#define LPP_DATATYPE_BAROMETER 0x73
#define LPP_DATATYPE_BAROMETER_SIZE 2
#define LPP_DATATYPE_FRAME_IDENTIFIER 0x99  // identifier of data frame (who is it coming from)

/* Private variables ---------------------------------------------------------*/

static sensor_t Sensor;                            /* struct for data sensor*/

#if defined(USE_LRWAN_NS1)
/* 以两位小数打印浮点数，不使用 %f */
static void print_2dec(const char *label, float value, const char *unit)
{
  int v100 = (int)(value * 100.0f);
  int v_abs = (v100 < 0) ? -v100 : v100;
  dbg_printf_send("%s = %s%d.%02d %s\n", label, (v100 < 0) ? "-" : "",
                  v_abs / 100, v_abs % 100, unit);
}
#endif



#if USE_LRWAN_NS1
static bool user_button_flag = true;
#endif


/* Private function prototypes -----------------------------------------------*/

static void SensorMeasureData(sSendDataBinary_t *SendDataBinary);

/* load call backs*/
static LoRaDriverCallback_t LoRaDriverCallbacks = { SensorMeasureData,
                                                    NULL
                                                  };

#if USE_LRWAN_NS1
/**
  * @brief  EXTI line detection callbacks.
  * @param  GPIO_Pin: Specifies the pins connected to the EXTI line.
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  /*HW_RTC_DelayMs(10);*/
  RTC_IF_DelayMs(10);
  if (!HAL_GPIO_ReadPin(USER_BUTTON_GPIO_PORT, USER_BUTTON_PIN))
  {
    if (user_button_flag)
    {
      Modem_IO_DeInit();
      BSP_LED_DeInit(LED2);
      user_button_flag = false;
      while (1);
    }
    else
    {
      Modem_IO_Init();
      NVIC_SystemReset();
    }
  }
}
#endif

/* Private macro ------------- -----------------------------------------------*/

#ifdef CAYENNE_LPP
#define LORAWAN_APP_PORT           99;            /*LoRaWAN application port*/
#else
#define LORAWAN_APP_PORT           2;            /*LoRaWAN application port*/
#endif

#define LORAWAN_CONFIRMED_MSG      ENABLE         /*LoRaWAN confirmed messages*/

#define SENSORS_MEASURE_CYCLE      15000          /*Periode to do sensors measurement*/

#define JOIN_MODE                  OTAA_JOIN_MODE   /*ABP_JOIN_MODE */ /*LoRaWan join methode*/

#ifdef USE_LRWAN_NS1
#define FREQ_BAND                  /*EU868*/ /*CN470PREQUEL*/ EU433 // YB: move from CN470PREQUEL to EU433 
#endif


/* Init LoRa Driver modem parameters*/
#ifdef USE_LRWAN_NS1
static LoRaDriverParam_t LoRaDriverParam = {  SENSORS_MEASURE_CYCLE,  JOIN_MODE, FREQ_BAND};
#else
static LoRaDriverParam_t LoRaDriverParam = {  SENSORS_MEASURE_CYCLE,  JOIN_MODE};
#endif



/*!
  * Master context initialization following the LoRa modem used
  *
  */
void MasterApp_Init(void)
{

  /* if using sequencer uncomment the task creation */
  /*UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_Lora_fsm), UTIL_SEQ_RFU, Lora_fsm); */

  Lora_Ctx_Init(&LoRaDriverCallbacks, &LoRaDriverParam);
}

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/********************* LoRa Part Apllication **********************************/

/******************************************************************************
  * @brief SensorMeasureData
  * @param none
  * @return none
  ******************************************************************************/
static void SensorMeasureData(sSendDataBinary_t *SendDataBinary)
{
#ifndef CAYENNE_LPP
  uint8_t LedState = 0;                /*just for padding*/
#endif
  // 1. TODO LORA USE_LRWAN_NS1: uncomment those variables below vvv

  uint16_t pressure = 0;

  uint8_t humidity = 0;

  int16_t temperature = 0;
  uint32_t BatLevel = 0;               // end device connected to external power source
  ATEerror_t LoraCmdRetCode;

  uint8_t index = 0;
  /*read pressure, Humidity and Temperature in order to be send on LoRaWAN*/
  EnvSensors_Read(&Sensor);

#ifdef CAYENNE_LPP
  // 1. TODO LORA USE_LRWAN_NS1: uncomment this variable below vvv too
  uint8_t cchannel = 0;

  // 1. TODO LORA USE_LRWAN_NS1: THEN: print pressure, temperature, humidity on terminal!
  // 1. TODO LORA USE_LRWAN_NS1: with two decimals precision 
  // 1. TODO LORA USE_LRWAN_NS1: #if defined()/#endif style 
  // 1. TODO LORA USE_LRWAN_NS1: hint: use dbg_printf_send() (where is it? how does it work?)

#if defined(USE_LRWAN_NS1)
  print_2dec("Pressure",    Sensor.pressure,    "hPa");
  print_2dec("Temperature", Sensor.temperature, "C");
  print_2dec("Humidity",    Sensor.humidity,    "%");
#endif


  // 6. TODO LORA: convert temperature, pressure, humidity to data for SendDataBinary->Buffer
  // 6. TODO LORA: hint: decidegrees, decahPas, double humidity percents
  // 6. TODO LORA: do the proper final type casts for each of those values!
 
  temperature = (int16_t)((Sensor.temperature >= 0.0f)
                          ? (Sensor.temperature * 10.0f + 0.5f)
                          : (Sensor.temperature * 10.0f - 0.5f));
  dbg_printf_send("temperature (deci-deg) = %d\r\n", temperature);


  pressure = (uint16_t)(Sensor.pressure * 10.0f + 0.5f);
  humidity = (uint8_t)(Sensor.humidity * 2.0f + 0.5f);

  // 7. TODO LORA: create your data payload as per defined in the practical work
  // 7. TODO LORA: you will write into the SendDataBinary->Buffer

  //id
  const char group_id[] = "MR_DG";
  uint8_t id_len = (uint8_t)(sizeof(group_id) - 1);
  SendDataBinary->Buffer[index++] = cchannel++;
  SendDataBinary->Buffer[index++] = LPP_DATATYPE_FRAME_IDENTIFIER;
  SendDataBinary->Buffer[index++] = id_len;
  for (uint8_t k = 0; k < id_len; k++)
  {
    SendDataBinary->Buffer[index++] = group_id[k];
  }

  SendDataBinary->Buffer[index++] = cchannel++;
  SendDataBinary->Buffer[index++] = LPP_DATATYPE_BAROMETER;
  SendDataBinary->Buffer[index++] = LPP_DATATYPE_BAROMETER_SIZE;
  SendDataBinary->Buffer[index++] = (pressure >> 8) & 0xFF;
  SendDataBinary->Buffer[index++] =  pressure & 0xFF;


  //temperature
  uint16_t t_raw = (uint16_t)temperature;
  SendDataBinary->Buffer[index++] = cchannel++;
  SendDataBinary->Buffer[index++] = LPP_DATATYPE_TEMPERATURE;
  SendDataBinary->Buffer[index++] = LPP_DATATYPE_TEMPERATURE_SIZE;
  SendDataBinary->Buffer[index++] = (t_raw >> 8) & 0xFF;//msb
  SendDataBinary->Buffer[index++] =  t_raw       & 0xFF;//lsb


  SendDataBinary->Buffer[index++] = cchannel++;
  SendDataBinary->Buffer[index++] = LPP_DATATYPE_HUMIDITY;
  SendDataBinary->Buffer[index++] = LPP_DATATYPE_HUMIDITY_SIZE;
  SendDataBinary->Buffer[index++] = humidity;



  //batterie
  uint8_t bat_pct = 0;
  LoraCmdRetCode = Lora_GetBatLevel(&BatLevel);
  if (LoraCmdRetCode == ATCTL_RET_CMD_VDD)
  {
    bat_pct = (uint8_t)((BatLevel * 100U) / 254U);
    dbg_printf_send("Battery = %d %%\r\n", bat_pct);
  }
  else
  {
    dbg_printf_send("Battery read KO (ret = %d)\r\n", (int)LoraCmdRetCode);
  }
  SendDataBinary->Buffer[index++] = cchannel++;
  SendDataBinary->Buffer[index++] = LPP_DATATYPE_DIGITAL_INPUT;
  SendDataBinary->Buffer[index++] = LPP_DATATYPE_DIGITAL_INPUT_SIZE;
  SendDataBinary->Buffer[index++] = bat_pct;

  //afficher la trame
  dbg_printf_send("Payload (%d bytes): ", index);
  for (uint8_t k = 0; k < index; k++)
  {
    dbg_printf_send("%02X ", (uint8_t)SendDataBinary->Buffer[k]);
  }
  dbg_printf_send("\r\n");

#else

  int32_t latitude, longitude = 0;     /*just for padding*/
  uint16_t altitudeGps = 0;            /*just for padding*/

  temperature = 234; // YB: example decidgrees
  pressure = 11111;  // YB: example daPascals
  humidity = 750;    // YB: example decipercents of humidity, but we want double of percents instead!

  latitude = Sensor.latitude;    /* not relevant*/
  longitude = Sensor.longitude;  /* not relevant*/

  /*fill up the send buffer*/
  SendDataBinary->Buffer[index++] = LedState;
  SendDataBinary->Buffer[index++] = (pressure >> 8) & 0xFF;
  SendDataBinary->Buffer[index++] =  pressure & 0xFF;
  SendDataBinary->Buffer[index++] = (temperature >> 8) & 0xFF;
  SendDataBinary->Buffer[index++] =  temperature & 0xFF;
  SendDataBinary->Buffer[index++] = (humidity >> 8) & 0xFF;
  SendDataBinary->Buffer[index++] =  humidity & 0xFF;

  /*get battery level of the modem (slave)*/
  LoraCmdRetCode = Lora_GetBatLevel(&BatLevel);
  if (LoraCmdRetCode == AT_OK)
  {
    DBG_PRINTF("Msg status = %d --> OK\n", BatLevel);
  }
  else
  {
    DBG_PRINTF("Msg status = %d --> KO\n", BatLevel);
  }
  SendDataBinary->Buffer[index++] = (uint8_t)BatLevel;

  /*remaining data just for padding*/
  SendDataBinary->Buffer[index++] = (latitude >> 16) & 0xFF;
  SendDataBinary->Buffer[index++] = (latitude >> 8) & 0xFF;
  SendDataBinary->Buffer[index++] = latitude & 0xFF;
  SendDataBinary->Buffer[index++] = (longitude >> 16) & 0xFF;
  SendDataBinary->Buffer[index++] = (longitude >> 8) & 0xFF;
  SendDataBinary->Buffer[index++] = longitude & 0xFF;
  SendDataBinary->Buffer[index++] = (altitudeGps >> 8) & 0xFF;
  SendDataBinary->Buffer[index++] = altitudeGps & 0xFF;

#endif  /*CAYENNE_LPP*/

  // 8. TODO LORA: This is the end of your custom data. Leave the rest below untouched 

  SendDataBinary->DataSize = index;
#ifndef USE_LRWAN_NS1
  SendDataBinary->Port = LORAWAN_APP_PORT;
#endif

#ifdef USE_I_NUCLEO_LRWAN1
  SendDataBinary->Ack = !LORAWAN_CONFIRMED_MSG;
#endif
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t *file, uint32_t line)
{

  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

}

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
