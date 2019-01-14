// читаем смс и если доступна новая команда по смс то выполняем ее
void ExecSmsCommand()
{ 
  if (gsm.NewSms)
  {
    if ((gsm.SmsNumber.indexOf(NumberRead(E_NUM1_SmsCommand)) > -1 ||                    // если обнаружено зарегистрированый номер
         gsm.SmsNumber.indexOf(NumberRead(E_NUM2_SmsCommand)) > -1 ||
         gsm.SmsNumber.indexOf(NumberRead(E_NUM3_SmsCommand)) > -1 ||
         gsm.SmsNumber.indexOf(NumberRead(E_NUM4_SmsCommand)) > -1
        )
        ||
        (NumberRead(E_NUM1_SmsCommand).startsWith("***")  &&                             // если нет зарегистрированных номеров (при первом включении необходимо зарегистрировать номера)
         NumberRead(E_NUM2_SmsCommand).startsWith("***")  &&
         NumberRead(E_NUM3_SmsCommand).startsWith("***")  &&
         NumberRead(E_NUM4_SmsCommand).startsWith("***")
         )
       )
    {
      gsm.SmsText.toLowerCase();                                                         // приводим весь текст команды к нижнему регистру что б было проще идентифицировать команду
      gsm.SmsText.trim();                                                                // удаляем пробелы в начале и в конце комманды

      if (gsm.SmsText.startsWith("*") || gsm.SmsText.startsWith("#"))                    // если сообщение начинается на * или # то это Ussd запрос
      {
        PlayToneDependMod();                                                             // подавать звуковой сигнал только если в режиме не на контроле
        if (gsm.RequestUssd(&gsm.SmsText))                                               // отправляем Ussd запрос и если он валидный (запрос заканчиваться на #)
          numberAnsUssd = gsm.SmsNumber;                                                 // то сохраняем номер на который необходимо будет отправить ответ от Ussd запроса
        else
          SendSms(&GetStrFromFlash(sms_WrongUssd), &gsm.SmsNumber);                      // иначе отправляем сообщение об инвалидном Ussd запросе
      }
      else
      if (gsm.SmsText.startsWith(GetStrFromFlash(sendsms)))                              // если обнаружена смс команда для отправки смс другому абоненту
      {
        PlayToneDependMod();                                                             // подавать звуковой сигнал только если в режиме не на контроле
        String number = "";                                                              // переменная для хранения номера получателя
        String text = "";                                                                // переменная для хранения текста смс
        String str = gsm.SmsText;

        // достаем номер телефона кому отправляем смс
        int beginStr = str.indexOf('\'');
        if (beginStr > 0)                                                                // проверяем, что прискутсвуют параметры SendSMS  команды (номер и текст перенаправляемой смс)
        {
          str = str.substring(beginStr + 1);                                             // достаем номер получателя
          int duration = str.indexOf('\'');
          number = str.substring(0, duration);
          str = str.substring(duration +1);

          beginStr = 0;                                                                  // достаем текст смс, который необходимо отправить получателю
          duration = 0;
          beginStr = str.indexOf('\'');
          str = str.substring(beginStr + 1);
          duration = str.indexOf('\'');
          text = str.substring(0, duration);
         }
        number.trim();
        if (number.length() > 0)                                                         // проверяем что номер получателя не пустой (смс текст не проверяем так как перенаправление пустого смс возможно)
        {
          if(SendSms(&text, &number))                                                    // перенаправляем смс указанному получателю и если сообщение перенаправлено успешно
            SendSms(&GetStrFromFlash(sms_SmsWasSent), &gsm.SmsNumber);                   // то отправляем отчет об успешном выполнении комманды
        }
        else
          SendSms(&GetStrFromFlash(sms_ErrorSendSms), &gsm.SmsNumber);                   // если номер получателя не обнаружен (пустой) то отправляем сообщение с правильным форматом комманды
      }
      else
      if (gsm.SmsText == GetStrFromFlash(balance))                                       // если обнаружена смс команда для запроса баланса
      {
        PlayToneDependMod();                                                             // подавать звуковой сигнал только если в режиме не на контроле
        if(gsm.RequestUssd(&ReadStrEEPROM(E_BalanceUssd)))                               // отправляем Ussd запрос для получения баланса и если он валидный (запрос заканчиваться на #)
          numberAnsUssd = gsm.SmsNumber;                                                 // то сохраняем номер на который необходимо будет отправить баланс
        else
          SendSms(&GetStrFromFlash(sms_WrongUssd), &gsm.SmsNumber);                      // иначе отправляем сообщение об инвалидном Ussd запросе
      }
      else
      if (gsm.SmsText == GetStrFromFlash(teston))                                        // если обнаружена смс команда для включения тестового режима для тестирования датчиков
      {
        PlayToneDependMod();                                                             // подавать звуковой сигнал только если в режиме не на контроле
        InTestMod(1);                                                                    // включаем тестовый режим
        SendSms(&GetStrFromFlash(sms_TestModOn), &gsm.SmsNumber);                        // отправляем смс о завершении выполнения даной смс команды
      }
      else
      if (gsm.SmsText == GetStrFromFlash(testoff))                                       // если обнаружена смс команда для выключения тестового режима для тестирования датчиков
      {
        PlayToneDependMod();                                                             // подавать звуковой сигнал только если в режиме не на контроле
        InTestMod(0);                                                                    // выключаем тестовый режим
        SendSms(&GetStrFromFlash(sms_TestModOff), &gsm.SmsNumber);                       // отправляем смс о завершении выполнения даной смс команды
      }
      else
      if (gsm.SmsText == GetStrFromFlash(controlon))                                     // если обнаружена смс команда для установки на охрану
      {
        Set_OnContrMod(false, 0);                                                           // устанавливаем на охрану без паузы
        SendSms(&GetStrFromFlash(sms_OnContrMod), &gsm.SmsNumber);                       // отправляем смс о завершении выполнения даной смс команды
      }
      else
      if (gsm.SmsText == GetStrFromFlash(controloff))                                    // если обнаружена смс команда для снятие с охраны
      {
        Set_OutOfContrMod(0);                                                            // снимаем с охраны
        SendSms(&GetStrFromFlash(sms_OutOfContrMod), &gsm.SmsNumber);                    // отправляем смс о завершении выполнения даной смс команды
      }
      else
      if (gsm.SmsText == GetStrFromFlash(redirecton))                                    // если обнаружена смс команда для включения режима "перенапралять входящие смс от незарегистрированных номеров на номер SmsCommand1"
      {
        PlayToneDependMod();                                                             // подавать звуковой сигнал только если в режиме не на контроле
        EEPROM.write(E_isRedirectSms, true);
        SendSms(&GetStrFromFlash(sms_RedirectOn), &gsm.SmsNumber);
      }
      else
      if (gsm.SmsText == GetStrFromFlash(redirectoff))                                   // если обнаружена смс команда для выключения режима "перенапралять входящие смс от незарегистрированных номеров на номер SmsCommand1"
      {
        PlayToneDependMod();                                                             // подавать звуковой сигнал только если в режиме не на контроле
        EEPROM.write(E_isRedirectSms, false);
        SendSms(&GetStrFromFlash(sms_RedirectOff), &gsm.SmsNumber);
      }
      else
      if (gsm.SmsText == GetStrFromFlash(skimpy))                                        // если обнаружена смс команда для кратковременного включения сирены (для теститования сирены)
      {
        SkimpySiren();
        SendSms(&GetStrFromFlash(sms_SkimpySiren), &gsm.SmsNumber);
      }
      else
      if (gsm.SmsText == GetStrFromFlash(reboot))                                        // если обнаружена смс команда для перезагрузки устройства
      {
        PlayToneDependMod();                                                             // подавать звуковой сигнал только если в режиме не на контроле
        WDRebooted = SMSRebooted;                                                        // устанавливаем флаг, указывающий на причину перезагрузки - по требованию sms команды
        WriteStrEEPROM(E_NUM_RebootAns, &gsm.SmsNumber);                                 // то сохраняем номер на который необходимо будет отправить после перезагрузки сообщение о перезагрузке устройства
        Reboot();                                                                        // перезагружаем устройство испульзуя WatchDog. После перезагрузки во время запуска устройства, gsm модуль тоже будет перезагружен
      }
      else
      if (gsm.SmsText == GetStrFromFlash(_status))                                       // если обнаружена смс команда для запроса статуса режимов и настроек устройства
      {
        PlayToneDependMod();                                                             // подавать звуковой сигнал только если в режиме не на контроле
        String msg = GetStrFromFlash(control)          + String((mode == OnContrMod) ? GetStrFromFlash(on) : GetStrFromFlash(off)) + "\n"
                   + GetStrFromFlash(test)             + String((inTestMod) ? GetStrFromFlash(on) : GetStrFromFlash(off)) + "\n"
                   + GetStrFromFlash(redirSms)         + String((EEPROM.read(E_isRedirectSms)) ? GetStrFromFlash(on) : GetStrFromFlash(off)) + "\n"
                   + GetStrFromFlash(power)            + ((powCtr.IsBattPower) ? "battery" : "network") + "\n"
                   + GetStrFromFlash(delSiren)         + String(EEPROM.read(E_delaySiren)) + GetStrFromFlash(sec);

        if (!SirEnabled)
          msg = msg + "\n" + GetStrFromFlash(sirenoff);
        msg = msg + "\n" +  GetStrFromFlash(FirmwareVer);                                     // выводим версию прошивки
        unsigned long ltime;
        String sStatus = "";
        if (EEPROM.read(E_IsGasEnabled))
        {
          if (!SenGas.IsReady) sStatus = GetStrFromFlash(GasNotReady);                        // если датчик газа/дыма еще не прогрет то информируем об этом
          else
          {
            if (SenGas.PrTrigTime == 0) sStatus = GetStrFromFlash(idle);
            else
            {
              ltime = GetElapsed(SenGas.PrTrigTime)/1000;
              if (ltime <= 180) sStatus = String(ltime) + GetStrFromFlash(sec);               // < 180 сек.
              else
              if (ltime <= 7200) sStatus = String(ltime / 60) + GetStrFromFlash(minut);       // < 120 мин.
              else
              sStatus = String(ltime / 3600) + GetStrFromFlash(hour);
            }
            msg = msg + "\n" + GetStrFromFlash(Gas) + sStatus + "\n"
                      + GetStrFromFlash(GasVal) + String(GasPct) + "%";
          }
        }
        if (mode == OnContrMod)
        {
          if (EEPROM.read(E_IsPIR1Enabled))
          {
            if (SenPIR1.PrTrigTime == 0) sStatus = GetStrFromFlash(idle);
            else
            {
              ltime = GetElapsed(SenPIR1.PrTrigTime)/1000;
              if (ltime <= 180) sStatus = String(ltime) + GetStrFromFlash(sec);             // < 180 сек.
              else
              if (ltime <= 7200) sStatus = String(ltime / 60) + GetStrFromFlash(minut);     // < 120 мин.
              else
              sStatus = String(ltime / 3600) + GetStrFromFlash(hour);
            }
            msg = msg + "\n" + GetStrFromFlash(PIR1) + sStatus;
          }
          if (EEPROM.read(E_IsPIR2Enabled))
          {
            if (SenPIR2.PrTrigTime == 0) sStatus = GetStrFromFlash(idle);
            else
            {
              ltime = GetElapsed(SenPIR2.PrTrigTime)/1000;
              if (ltime <= 180) sStatus = String(ltime) + GetStrFromFlash(sec);             // < 180 сек.
              else
              if (ltime <= 7200) sStatus = String(ltime / 60) + GetStrFromFlash(minut);     // < 120 мин.
              else
              sStatus = String(ltime / 3600) + GetStrFromFlash(hour);
            }
            msg = msg + "\n" + GetStrFromFlash(PIR2) + sStatus;
          }
          if (EEPROM.read(E_TensionEnabled))
          {
            if (SenTension.PrTrigTime == 0) sStatus = GetStrFromFlash(idle);
            else
            {
              ltime = GetElapsed(SenTension.PrTrigTime)/1000;
              if (ltime <= 180) sStatus = String(ltime) + GetStrFromFlash(sec);             // < 180 сек.
              else
              if (ltime <= 7200) sStatus = String(ltime / 60) + GetStrFromFlash(minut);     // < 120 мин.
              else
              sStatus = String(ltime / 3600) + GetStrFromFlash(hour);
            }
            msg = msg + "\n" + GetStrFromFlash(tension) + sStatus;
          }
        }
        SendSms(&msg, &gsm.SmsNumber);
      }
      else
      if (gsm.SmsText == GetStrFromFlash(setting) || gsm.SmsText == (GetStrFromFlash(setting)+"s"))       //  если обнаружена команда для возврата сетингов (команда setting или settings)
      {
        PlayToneDependMod();                                                                              // подавать звуковой сигнал только если в режиме не на контроле
        SendInfSMS_Setting();
      }
      else
      if (gsm.SmsText == GetStrFromFlash(button) || gsm.SmsText == (GetStrFromFlash(button)+"s"))         // если обнаружена команда для возврата сетингов кнопки
      {
        PlayToneDependMod();                                                                              // подавать звуковой сигнал только если в режиме не на контроле
        SendInfSMS_Button();
      }
      else
      if (gsm.SmsText == GetStrFromFlash(sensor) || gsm.SmsText == (GetStrFromFlash(sensor)+"s"))         // если обнаружена команда для возврата сетингов датчиков
      {
        PlayToneDependMod();                                                                              // подавать звуковой сигнал только если в режиме не на контроле
        SendInfSMS_Sensor();
      }
      else
      if (gsm.SmsText == GetStrFromFlash(siren))                                                          // если обнаружена команда для возврата сетингов датчиков
      {
        PlayToneDependMod();                                                                              // подавать звуковой сигнал только если в режиме не на контроле
        SendInfSMS_Siren();
      }
      else
      if (gsm.SmsText == GetStrFromFlash(outofcontr))                                                     // если обнаружена смс команда для запроса списка зарегистрированных телефонов для снятие с охраны
      {
        PlayToneDependMod();                                                                              // подавать звуковой сигнал только если в режиме не на контроле
        SendInfSMS_OutOfContr();
      }
      else
      if (gsm.SmsText == GetStrFromFlash(oncontr))                                                        // если обнаружена смс команда для запроса списка зарегистрированных телефонов для установки на охрану
      {
        PlayToneDependMod();                                                                              // подавать звуковой сигнал только если в режиме не на контроле
        SendInfSMS_OnContr();
      }
      else
      if (gsm.SmsText == GetStrFromFlash(smscommand))                                                     // если обнаружена смс команда для запроса списка зарегистрированных телефонов для управления через смс команды
      {
        PlayToneDependMod();                                                                              // подавать звуковой сигнал только если в режиме не на контроле
        SendInfSMS_SmsCommand();
      }
      else
      if (gsm.SmsText.startsWith(GetStrFromFlash(btnoncontr)))                                            // если обнаружена команда для настройки кнопки
      {
        PlayToneDependMod();                                                                              // подавать звуковой сигнал только если в режиме не на контроле
        String str = gsm.SmsText;
        byte bConf[5];                                                                                    // сохраняем настройки по кнопке
        for(byte i = 0; i < 5; i++)
        {
          int beginStr = str.indexOf('\'');
          str = str.substring(beginStr + 1);
          int duration = str.indexOf('\'');
          bConf[i] = (str.substring(0, duration)).toInt();
          str = str.substring(duration +1);
        }
        EEPROM.write(E_BtnOnContr, bConf[0]);
        EEPROM.write(E_BtnInTestMod, bConf[1]);
        EEPROM.write(E_BtnBalance, bConf[2]);
        EEPROM.write(E_BtnSkimpySiren, bConf[3]);
        EEPROM.write(E_BtnOutOfContr, bConf[4]);
        SendInfSMS_Button();
      }
      else
      if (gsm.SmsText.startsWith(GetStrFromFlash(delaySiren)))                                            // если обнаружена команда с основными настройками устройства (сетинги)
      {
        PlayToneDependMod();                                                                              // подавать звуковой сигнал только если в режиме не на контроле
        String sConf[6];
        String str = gsm.SmsText;
        for(byte i = 0; i < 6; i++)
        {
          int beginStr = str.indexOf('\'');
          str = str.substring(beginStr + 1);
          int duration = str.indexOf('\'');
          if (i < 4)
            sConf[i] = str.substring(0, duration);
          else if (i == 4)
          {
            if (str.substring(0, duration) == "off")
              EEPROM.write(E_infContr, false);
            else if (str.substring(0, duration) == "on")
              EEPROM.write(E_infContr, true);
          }
          else if (i == 5)
          {
            if (str.substring(0, duration) == "off")
            {
              EEPROM.write(E_gasOnlyOnContr, false);
              if (EEPROM.read(E_IsGasEnabled)) SenGas.TurnOnPower();                                       // если датчик газа активирован то включаем его
            }
            else if (str.substring(0, duration) == "on")
            {
              EEPROM.write(E_gasOnlyOnContr, true);
              if (mode == OutOfContrMod) SenGas.TurnOffPower();                                            // если режим не на охране то выключаем датчик газа
            }
          }
          str = str.substring(duration + 1);
        }
        EEPROM.write(E_delaySiren, (byte)sConf[0].toInt());
        EEPROM.write(E_delayOnContr, (byte)sConf[1].toInt());
        EEPROM.write(E_delayVcc, (byte)sConf[2].toInt());
        WriteStrEEPROM(E_BalanceUssd, &sConf[3]);
        SendInfSMS_Setting();
      }
      else
      if (gsm.SmsText.startsWith(GetStrFromFlash(_PIR1)))                                                  // если обнаружена команда с настройками датчиков
      {
        PlayToneDependMod();                                                                               // подавать звуковой сигнал только если в режиме не на контроле
        String str = gsm.SmsText;
        bool bConf[4];                                                                                     // сохраняем настройки по датчикам
        for(byte i = 0; i < 5; i++)
        {
          int beginStr = str.indexOf('\'');
          str = str.substring(beginStr + 1);
          int duration = str.indexOf('\'');
          if (i < 4)
          {
            if (str.substring(0, duration) == "off")
              bConf[i] = false;
            else if (str.substring(0, duration) == "on")
              bConf[i] = true;
          }
          else if (i == 4)
            SenGas.gasClbr = (str.substring(0, duration)).toInt();
          str = str.substring(duration +1);
        }
        if (!EEPROM.read(E_IsGasEnabled) && bConf[2])                                                      // если датчик газа был выключен и теперь его вклчают
        {
          SenGas.TurnOnPower();                                                                            // включаем питание датчика газа/дыма подавая питание на ногу pinGasPower и используя Мосфет транзистор
        }
        else if (!bConf[2])                                                                                // если выключают датчик газа/дыма
        {
           SenGas.TurnOffPower();                                                                          // то выключаем ему питание
        }
        EEPROM.write(E_IsPIR1Enabled, bConf[0]);
        EEPROM.write(E_IsPIR2Enabled, bConf[1]);
        EEPROM.write(E_IsGasEnabled,  bConf[2]);
        EEPROM.write(E_TensionEnabled, bConf[3]);
        WriteIntEEPROM(E_gasCalibr, SenGas.gasClbr);
        SendInfSMS_Sensor();
      }
      else
      if (gsm.SmsText.startsWith(GetStrFromFlash(_SirenEnabled)))                                          // если обнаружена команда с настройками сирены
      {
        PlayToneDependMod();                                                                               // подавать звуковой сигнал только если в режиме не на контроле
        String str = gsm.SmsText;
        bool bConf[4];                                                                                     // сохраняем настройки по сирене
        for(byte i = 0; i < 4; i++)
        {
          int beginStr = str.indexOf('\'');
          str = str.substring(beginStr + 1);
          int duration = str.indexOf('\'');
          if (str.substring(0, duration) == "off")
            bConf[i] = false;
          else if (str.substring(0, duration) == "on")
            bConf[i] = true;
          str = str.substring(duration +1);
        }
        SirEnabled = bConf[0];
        PIR1Sir = bConf[1];
        PIR2Sir = bConf[2];
        TensionSir = bConf[3];
        EEPROM.write(E_SirenEnabled, SirEnabled);
        EEPROM.write(E_PIR1Siren, PIR1Sir);
        EEPROM.write(E_PIR2Siren, PIR2Sir);
        EEPROM.write(E_TensionSiren, TensionSir);

        SendInfSMS_Siren();
      }
      else
      if (gsm.SmsText.startsWith(GetStrFromFlash(outofcontr1)))                                            // если обнаружена смс команда для регистрации группы телефонов для снятие с охраны
      {
        PlayToneDependMod();                                                                               // подавать звуковой сигнал только если в режиме не на контроле
        String nums[4];
        String str = gsm.SmsText;;
        for(byte i = 0; i < 4; i++)
        {
          int beginStr = str.indexOf('\'');
          str = str.substring(beginStr + 1);
          int duration = str.indexOf('\'');
          nums[i] = str.substring(0, duration);
          str = str.substring(duration +1);
        }
        WriteStrEEPROM(E_NUM1_OutOfContr, &nums[0]);
        WriteStrEEPROM(E_NUM2_OutOfContr, &nums[1]);
        WriteStrEEPROM(E_NUM3_OutOfContr, &nums[2]);
        WriteStrEEPROM(E_NUM4_OutOfContr, &nums[3]);
        SendInfSMS_OutOfContr();
      }
      else
      if (gsm.SmsText.startsWith(GetStrFromFlash(oncontr1)))                                               // если обнаружена смс команда для регистрации группы телефонов для установки на охрану
      {
        PlayToneDependMod();                                                                               // подавать звуковой сигнал только если в режиме не на контроле
        String nums[3];
        String str = gsm.SmsText;
        for(byte i = 0; i < 3; i++)
        {
          int beginStr = str.indexOf('\'');
          str = str.substring(beginStr + 1);
          int duration = str.indexOf('\'');
          nums[i] = str.substring(0, duration);
          str = str.substring(duration +1);
        }
        WriteStrEEPROM(E_NUM1_OnContr, &nums[0]);
        WriteStrEEPROM(E_NUM2_OnContr, &nums[1]);
        WriteStrEEPROM(E_NUM3_OnContr, &nums[2]);
        SendInfSMS_OnContr();
      }
      else
      if (gsm.SmsText.startsWith(GetStrFromFlash(smscommand1)))                                            // если обнаружена смс команда для регистрации группы телефонов для управления через смс команды
      {
        PlayToneDependMod();                                                                               // подавать звуковой сигнал только если в режиме не на контроле
        String nums[4];
        String str = gsm.SmsText;
        for(byte i = 0; i < 4; i++)
        {
          int beginStr = str.indexOf('\'');
          str = str.substring(beginStr + 1);
          int duration = str.indexOf('\'');
          nums[i] = str.substring(0, duration);
          str = str.substring(duration +1);
        }
        WriteStrEEPROM(E_NUM1_SmsCommand, &nums[0]);
        WriteStrEEPROM(E_NUM2_SmsCommand, &nums[1]);
        WriteStrEEPROM(E_NUM3_SmsCommand, &nums[2]);
        WriteStrEEPROM(E_NUM4_SmsCommand, &nums[3]);
        SendInfSMS_SmsCommand();
      }
      else                                                                                                 // если смс команда не распознана
      {
        PlayToneDependMod();                                                                               // подавать звуковой сигнал только если в режиме не на контроле
        SendSms(&GetStrFromFlash(sms_ErrorCommand), &gsm.SmsNumber);                                       // то отправляем смс со списком всех доступных смс команд
      }
    }
    else if (EEPROM.read(E_isRedirectSms))                                                                 // если смс пришла не с зарегистрированых номеров и включен режим перенаправления всех смс
    {
      SendSms(&String(gsm.SmsText), &NumberRead(E_NUM1_OutOfContr));                                       // перенаправляем смс на зарегистрированный номер под именем SmsCommand1
    }
  gsm.ClearSms();
  }
}

void SendInfSMS_Setting()
{
  String msg = GetStrFromFlash(delSiren)     + "'" + String(EEPROM.read(E_delaySiren))   + "'" + GetStrFromFlash(sec) + "\n"
     + GetStrFromFlash(delOnContr)           + "'" + String(EEPROM.read(E_delayOnContr)) + "'" + GetStrFromFlash(sec) + "\n"
     + GetStrFromFlash(delayVcc)             + "'" + String(EEPROM.read(E_delayVcc))     + "'" + GetStrFromFlash(sec) + "\n"
     + GetStrFromFlash(balanceUssd)          + "'" + ReadStrEEPROM(E_BalanceUssd)        + "'" + "\n"
     + GetStrFromFlash(infContr)             + "'" + String((EEPROM.read(E_infContr)) ? "on" : "off") + "'";
  if (EEPROM.read(E_IsGasEnabled))
    msg = msg + "\n" + GetStrFromFlash(gasOnlyOnContr)  + "'" + String((EEPROM.read(E_gasOnlyOnContr)) ? "on" : "off") + "'";

  SendSms(&msg, &gsm.SmsNumber);
}

void SendInfSMS_Button()
{
  String msg = GetStrFromFlash(BtnOnContr )  + "'" + String((EEPROM.read(E_BtnOnContr)))     + "'" + "\n"
    + GetStrFromFlash(BtnInTestMod)          + "'" + String((EEPROM.read(E_BtnInTestMod)))   + "'" + "\n"
    + GetStrFromFlash(BtnBalance)            + "'" + String((EEPROM.read(E_BtnBalance)))     + "'" + "\n"
    + GetStrFromFlash(BtnSkimpySiren)        + "'" + String((EEPROM.read(E_BtnSkimpySiren))) + "'" + "\n"
    + GetStrFromFlash(BtnOutOfContr)         + "'" + String((EEPROM.read(E_BtnOutOfContr)))  + "'";
  SendSms(&msg, &gsm.SmsNumber);
}

void SendInfSMS_Sensor()
{
  String msg = GetStrFromFlash(PIR1)         + "'" + String((EEPROM.read(E_IsPIR1Enabled))  ? GetStrFromFlash(on) : GetStrFromFlash(off)) + "'" + "\n"
     + GetStrFromFlash(PIR2)                 + "'" + String((EEPROM.read(E_IsPIR2Enabled))  ? GetStrFromFlash(on) : GetStrFromFlash(off)) + "'" + "\n"
     + GetStrFromFlash(Gas)                  + "'" + String((EEPROM.read(E_IsGasEnabled))   ? GetStrFromFlash(on) : GetStrFromFlash(off)) + "'" + "\n"
     + GetStrFromFlash(tension)              + "'" + String((EEPROM.read(E_TensionEnabled)) ? GetStrFromFlash(on) : GetStrFromFlash(off)) + "'" + "\n"
     + GetStrFromFlash(GasCalibr)            + "'" + String(SenGas.gasClbr) + "'" + "\n"
     + GetStrFromFlash(GasCurr)              + "'" + String((SenGas.IsReady) ? String(SenGas.GetSensorValue()) : GetStrFromFlash(GasNotReady)) + "'";
  SendSms(&msg, &gsm.SmsNumber);
}

void SendInfSMS_Siren()
{
  String msg = GetStrFromFlash(SirenEnabled)       + "'" + String((SirEnabled) ? GetStrFromFlash(on) : GetStrFromFlash(off)) + "'" + "\n"
     + GetStrFromFlash(PIR1Siren)                  + "'" + String((PIR1Sir)    ? GetStrFromFlash(on) : GetStrFromFlash(off)) + "'" + "\n"
     + GetStrFromFlash(PIR2Siren)                  + "'" + String((PIR2Sir)    ? GetStrFromFlash(on) : GetStrFromFlash(off)) + "'" + "\n"
     + GetStrFromFlash(TensionSiren)               + "'" + String((TensionSir) ? GetStrFromFlash(on) : GetStrFromFlash(off)) + "'";
  SendSms(&msg, &gsm.SmsNumber);
}

void SendInfSMS_OutOfContr()
{
  String msg = "OutOfContr1:\n'" + NumberRead(E_NUM1_OutOfContr) + "'" + "\n"
             + "OutOfContr2:\n'" + NumberRead(E_NUM2_OutOfContr) + "'" + "\n"
             + "OutOfContr3:\n'" + NumberRead(E_NUM3_OutOfContr) + "'" + "\n"
             + "OutOfContr4:\n'" + NumberRead(E_NUM4_OutOfContr) + "'";
  SendSms(&msg, &gsm.SmsNumber);
}

void SendInfSMS_OnContr()
{
  String msg = "OnContr1:\n'" + NumberRead(E_NUM1_OnContr) + "'" + "\n"
             + "OnContr2:\n'" + NumberRead(E_NUM2_OnContr) + "'" + "\n"
             + "OnContr3:\n'" + NumberRead(E_NUM3_OnContr) + "'";
  SendSms(&msg, &gsm.SmsNumber);
}

void SendInfSMS_SmsCommand()
{
  String msg = "SmsCommand1:\n'" + NumberRead(E_NUM1_SmsCommand) + "'" + "\n"
             + "SmsCommand2:\n'" + NumberRead(E_NUM2_SmsCommand) + "'" + "\n"
             + "SmsCommand3:\n'" + NumberRead(E_NUM3_SmsCommand) + "'" + "\n"
             + "SmsCommand4:\n'" + NumberRead(E_NUM4_SmsCommand) + "'";
  SendSms(&msg, &gsm.SmsNumber);
}


void PlayToneDependMod()
{
  if (mode == OutOfContrMod || inTestMod) PlayTone(sysTone, smsSpecDur);
}


