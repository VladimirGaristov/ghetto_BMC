#include <ESP8266WiFi.h>
#include "Ghetto_BMC.h"
#include "aes.hpp"

WiFiServer BMC_server(SERVER_PORT);
WiFiClient BMC_client;

struct AES_ctx enc_ctx;
int expected_packet = HELLO_PACKET;
int32_t nonce = 0;
uint8_t cmd = 0, resp_code = 0, resp_type;

void setup()
{
  pinMode(RST_PIN, OUTPUT);
  digitalWrite(RST_PIN, LOW);
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  pinMode(V5_SENSE_PIN, INPUT);
  pinMode(A0, INPUT);
  
  Serial.begin(115200);
  delay(10);
  Serial.println("\nAttempting to connect...");

  AES_init_ctx(&enc_ctx, (const uint8_t*) aes_key);

  int i = 0;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(++i);
    Serial.print(' ');
  }
  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());

  BMC_server.begin();
}

void loop()
{
  uint8_t inp_buffer[BLOCK_SIZE] = {0}, out_buffer[BLOCK_SIZE] = {0};
  int i;
  if (BMC_client)
  {
    if (BMC_client.available())
    {
      if (expected_packet == HELLO_PACKET)
      {
        inp_buffer[0] = BMC_client.read();
        if (inp_buffer[0] == HELLO_PACKET)
        {
          expected_packet = COMMAND_PACKET;
          
          //Generate nonce
          randomSeed(millis() | (unsigned long int) analogRead(A0));
          nonce = (int32_t) random(RAND_MAX);
          Serial.println(nonce);

          //Send nonce
          out_buffer[0] = NONCE_PACKET;
          *(int32_t*) (out_buffer + 1) = nonce;
          for (i=5; i < BLOCK_SIZE; i++)
          {
            *(uint8_t*) (out_buffer + i) = (uint8_t) random(RAND_MAX);
          }
          AES_ECB_encrypt(&enc_ctx, out_buffer);
          BMC_client.write(out_buffer, BLOCK_SIZE);
        }
      }
      else if (expected_packet == COMMAND_PACKET)
      {
        expected_packet = HELLO_PACKET;
        resp_type = ERROR_PACKET;   //by default

        //Read packet
        for (i = 0; i < BLOCK_SIZE; i++)
        {
          inp_buffer[i] = BMC_client.read();
        }
        AES_ECB_decrypt(&enc_ctx, inp_buffer);

        //Check type and nonce
        if (inp_buffer[0] != COMMAND_PACKET)
        {
          resp_code = WRONG_PKT_TYPE;
        }
        else if (*(int32_t*) (inp_buffer + 2) != nonce)
        {
          resp_code = WRONG_NONCE;
        }

        //Decode and execute command
        else
        {
          resp_type = SUCCESS_PACKET;
          switch (inp_buffer[1])
          {
            case STATUS:
              Serial.println(digitalRead(V5_SENSE_PIN));
              if (digitalRead(V5_SENSE_PIN))
              {
                resp_code = PWR_ON;
              }
              else
              {
                resp_code = PWR_OFF;
              }
              break;
            case START:
              digitalWrite(PWR_PIN, HIGH);
              delay(10);
              digitalWrite(PWR_PIN, LOW);
              resp_code = CMD_EXECUTED;
              break;
            case SHUTDOWN:
              digitalWrite(PWR_PIN, HIGH);
              delay(6000);
              digitalWrite(PWR_PIN, LOW);
              resp_code = CMD_EXECUTED;
              break;
            case REBOOT:
              digitalWrite(RST_PIN, HIGH);
              delay(10);
              digitalWrite(RST_PIN, LOW);
              resp_code = CMD_EXECUTED;
              break;
            default:
              resp_code = INVALID_CMD;
              resp_type = ERROR_PACKET;
          }
        }
        
        //Send response packet
        out_buffer[0] = resp_type;
        *(int32_t*) (out_buffer + 1) = nonce;
        out_buffer[5] = resp_code;
        for (i=6; i < BLOCK_SIZE; i += 2)
        {
          *(uint16_t*) (out_buffer + i) = (uint16_t) random(RAND_MAX);
        }
        AES_ECB_encrypt(&enc_ctx, out_buffer);
        BMC_client.write(out_buffer, BLOCK_SIZE);
        BMC_client.stop();
      }
    }
  }
  else
  {
    BMC_client = BMC_server.available();
  }
}
