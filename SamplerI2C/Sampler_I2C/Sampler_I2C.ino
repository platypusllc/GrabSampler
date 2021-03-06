#include <Wire.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>


SoftwareSerial SoftSerial(10, 11); //rx tx
int numPumps = 4;
int pumpAddr[] = {1,2,3,4};
double cur_vol[4];
bool active[] = {true,true,true,true}; //can pump be used
bool finished[] = {false,false,false,false};
char delimiters[] = "{:\"}";
char command_keys[] = "edsrcv";
char numerics[] = "0123456789.-";


const size_t INPUT_BUFFER_SIZE = 512;
char input_buffer[INPUT_BUFFER_SIZE+1];
int input_index = 0;
const size_t OUTPUT_BUFFER_SIZE = 512;
char output_buffer[OUTPUT_BUFFER_SIZE+1];

int vol = 400;
//int vol = 1;

long currentTime;

void setup()
{
  Serial.begin(9600); //transfer from eboard to samplerard
  SoftSerial.begin(9600); //debug serial
  Wire.begin();
	// enable(0);
	// enable(1);
	// enable(2);
	// enable(3);
}

void loop()
{
	while (SoftSerial.available())
  {
    char c = SoftSerial.read();
    //Serial.println(c);
    if (c != '\n') // ignore new lines
    {
      input_buffer[input_index] = c;
      input_index++;
      if (input_index >= INPUT_BUFFER_SIZE)
      {
        Serial.println("filled buffer, return to index 0");
        input_index = 0;
        continue;
      }
    }    
    if (c == '\r') // command ends with carriage return
    {
      input_buffer[input_index] = '\0';
      Serial.print("-> ");
      Serial.println(input_buffer);        
      handleCommand(input_buffer);
      input_index = 0;
    }
  }

	/*Unfortunetly there is some error that arises with one of the pumps
	  That returns a ? symbol for the volume amount from pump 1 so for now
		we wont use this.*/
// if (currentTime - millis() >= 500)
//   {
//     feedbackLoop();
//   }
// currentTime = millis();

}

/*

  {e:0-4} //enables 0-4 (turn on)
  {d:0-4} //disables 0-4 (turn off)
  {s:} //stops all of them
  {r:} //resets all of them and puts them back in an active state
  {c:0-4} //get status of pump (either enabled or disabled)
  {v:ml} //sets how many ml of water to pump
  {f:0} //return finished status of a pump
*/


void handleCommand(char *buffer)
{

  // debug: force buffer to be full of gibberish
  //char debug_buffer[] = "s: a";
  //char debug_buffer[] = {0xFF, 0xF2, 0xFF, 0xFF, 0xF0, 0xF3, 0xF0, 0xFC};
  //buffer = debug_buffer;
  
  char *param_string;
  char *value_string;
  char *buffer_string_bookmark; // used for re-entrant version of strtok, strtok_r
  char *param_string_bookmark; // strtok_r lets you use strtok on more than one thing at a time 
  char *value_string_bookmark;
  int value;

  Serial.print("command: "); Serial.println(buffer);
  
  char* chunk;
  chunk = strtok_r(buffer, delimiters, &buffer_string_bookmark);
  int token_count = 0;  
  while (chunk != NULL && token_count < 2) // only expect a parameter: value JSON
  {
    switch (token_count)
    {
      case 0:
        param_string = chunk;        
        char *valid_command_check;
        valid_command_check = strtok_r(param_string, command_keys, &param_string_bookmark);
        if (valid_command_check != NULL)
        {
          Serial.print("Unknown command: "); Serial.print(chunk); Serial.println(", ignoring...");
          // TODO: blink a red LED
          return;
        }        
        Serial.print("param = "); Serial.println(param_string);
        break;
      case 1:
        value_string = chunk;
        Serial.print("value chunk = "); Serial.println(value_string);
        if (strtok_r(chunk, value_string, &value_string_bookmark) == NULL)
        {
          value = atoi(value_string);
        }
        else
        {
          Serial.println("WARNING: value is non-numeric, ignoring...");
          // TODO: blink a red LED
          return;
        }
        Serial.print("value = "); Serial.println(value);
        break;
      default:
        break;
    }   
    chunk = strtok_r(NULL, delimiters, &buffer_string_bookmark); // continue through the buffer
    token_count++;
  }
  if (token_count != 2)
  {
    Serial.println("Did not receive a parameter: value pair, ignoring...");
    // TODO: blink a red LED
    return;
  }
  char param = param_string[0];
  // TODO: blink a GREEN LED

  if (param == 'e')
  {
    // Serial.print(atoi(value));
    // Serial.print("\n");
    //int pump = atoi(value); //addresses start at 1 not 0
    //enable(value[0]-'0');
    //Serial.println("got here");
    enable(value);
  }
  if (param == 'd')
  {
    //Serial.print("disabling pump ");
    //Serial.print(atoi(value));
    //Serial.print(value);
    //Serial.print("\n");
    //disable(value[0]-'0');
    disable(value);
  }
  if (param == 's')
  {
    // Serial.print("stopping all pumps\n");
    stop();
  }
  if (param == 'r')
  {
    Serial.println("resetting all pumps");
    for (int i = 0; i < 4; i++)
    {
      active[i] = true;
      finished[i] = false;
    }
  }
  if (param == 'c')
  {
    //int pump = atoi(value);
    //int pump = value[0]-'0';


    //Serial.print("<-");
    if (active[value] == true)
    {
      char str[500];
      snprintf(str,500,"{e:%d\r",value);
      SoftSerial.print(str); //Meaning you can use it
    }
    else
    {
      char str[500];
      snprintf(str,500,"{d:%d\r",value);
      SoftSerial.print(str);//Meaning you cannot use it
    }
  }
  if (param == 'v')
  {
    vol = value;
    Serial.print("volume is now: ");
    Serial.println(value);
  }
  // if (param == 'f')
  //   {
  //     char str[500];
  //     snprintf(str,500,"{%d,%d}\r",value,finished[value]);
  //     Serial.println(str);
  //     send(str);
  //   }
  
}
/*
  Replace the cont flow with volumetric flow eventually
  Wire.write("D,");
  Wire.Write(vol);
  Wire.write("\r");
*/
void enable(int pump)
{
  // Serial.print("pump is: ");
  // Serial.println(pump);

  if (pump < 0 || pump > 3)
  {
    Serial.println("Pump is out of range");
    return;
  }
  //{"s0":{e:0}}\r
  if (active[pump] == true)
  {
    //Serial.println("pump is active");
    char str[500];
    Wire.beginTransmission(pumpAddr[pump]);
    snprintf(str,20,"D,%d\r",vol);
    Wire.write(str);
    Wire.endTransmission();
    active[pump] = false;
    delay(300);
  }
}
void disable(int pump)
{
  if (pump < 0 || pump > 3)
  {
    Serial.println("Pump is out of range");
    return;
  }
  // Serial.print("disabling pump");
  // Serial.print(pump);
  Wire.beginTransmission(pumpAddr[pump]);
  Wire.write("X,\r");
  Wire.endTransmission();
  delay(300);
  active[pump] = false;
}
void stop()
{
  for (int i=0; i<4; i++)
  {
    disable(i);
  }
}

void send(char *str)
{
  size_t len = strlen(str);
  str[len] = '\0';
  SoftSerial.print(str);
  // Serial.print("-> ");
  // Serial.println(str);
}

void feedbackLoop()
{
  //get status of all pumps

  for (int p = 0; p < 4; p++)
    {
      if (finished[p] == true)
      {
        continue;
      }

      Wire.beginTransmission(pumpAddr[p]);
      Wire.write("R, \r");
      Wire.endTransmission();
      delay(300);
      Wire.requestFrom(p,30,1);

      char in_char;
      char pmp_data[30];
      int i = 0;
      while(Wire.available())
      {
        in_char = Wire.read();
        pmp_data[i]= in_char;
        i+=1;
        if (in_char==0)
        {
          i=0;
          Wire.endTransmission();
          break;
        }
      }
      double cur_amt = strtod(&pmp_data[1],NULL);
      char buf[500];
      snprintf(buf,500,"{%d:%s}\r",p,pmp_data);
      Serial.println(buf);

      cur_vol[i] = cur_amt;

      if (cur_amt == vol)
      {
        finished[p] = true;
      }
    }
}
