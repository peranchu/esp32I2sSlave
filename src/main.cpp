/*
 _    _ _    _ __  __          _   _   _      ______  _____ _____
| |  | | |  | |  \/  |   /\   | \ | | | |    |  ____|/ ____/ ____|
| |__| | |  | | \  / |  /  \  |  \| | | |    | |__  | (___| (___
|  __  | |  | | |\/| | / /\ \ | . ` | | |    |  __|  \___ \\___ \
| |  | | |__| | |  | |/ ____ \| |\  | | |____| |____ ____) |___) |
|_|  |_|\____/|_|  |_/_/    \_\_| \_| |______|______|_____/_____/

ESP32 - Esclavo Audio i2s comunicación esp_now
Honorino García Junio 2025

*/

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <AudioTools.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include "AudioTools/Disk/AudioSourceSD.h"
#include "AudioTools/AudioCodecs/CodecWAV.h"

// Conexiones SD
#define SD_CS 13
#define SPI_MOSI 15
#define SPI_MISO 2
#define SPI_SCK 14

const char *startFilePath = "/";
const char *ext = "wav";

// Cadena Audio
AudioSourceSD source(startFilePath, ext, SD_CS);
I2SStream i2s_play;
VolumeStream Vol_play(i2s_play);
WAVDecoder decoder;
AudioPlayer player(source, Vol_play, decoder);

bool playing = false;
bool FinRepro = false;
int message = 0;

float CStateVol = 0.0;
float PStateVol = 0.0;

typedef struct test_struct
{
    int play;
    float vol;
} test_struct;

test_struct myData;

void SDinit();
void config_i2s_play();

void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen)
{
    memcpy(&myData, data, sizeof(myData));

    CStateVol = myData.vol;

    // PLAY
    if (myData.play == 1)
    {
        Serial.println("Repro");
        message = 1;
    }

    // Volumen
    if (CStateVol != PStateVol)
    {
        player.setVolume(CStateVol);

        Serial.print("Volumen: ");
        Serial.println(CStateVol);

        PStateVol = CStateVol;
    }

    // Reset
    if (myData.play == 9)
    {
        Serial.println("Stop PLAY");

        ESP.restart();
    }
}
/////////////////////////////////

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;

    WiFi.mode(WIFI_STA);
    Serial.println("inicio Broadcast");
    WiFi.disconnect();

    if (esp_now_init() == ESP_OK)
    {
        Serial.println("ESP_NOW INICIADO");
        esp_now_register_recv_cb(receiveCallback);
    }
    else
    {
        Serial.println("ESP_NOW FAILED");
        delay(3000);
        ESP.restart();
    }

    SDinit();

    config_i2s_play();
}

void loop()
{
    if (message == 1)
    {
        if (!playing) // Si no está reproduciendo
        {
            Serial.println("config_play");

            config_i2s_play();

            player.begin();
            player.setVolume(0.7);

            playing = true;
        }
        if (!player) // Si ha terminado la reproducción
        {
            FinRepro = true;

            i2s_play.end();
            decoder.end();
            player.end();

            playing = false;
            message = 0;
            Serial.println("Stop PLAY");
        }
        else
        {
            player.copy();
        }
    }
}

// Inicio SD CARD
void SDinit()
{
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);

    if (!SD.begin(SD_CS))
    {
        Serial.println("Error Accediendo a SD!");
        while (true)
            ;
    }
}

// Inicio I2S
void config_i2s_play()
{
    auto cfg_play = i2s_play.defaultConfig(TX_MODE);
    cfg_play.i2s_format = I2S_STD_FORMAT;
    cfg_play.bits_per_sample = 16;
    cfg_play.sample_rate = 44100;
    cfg_play.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    cfg_play.channels = 1;
    cfg_play.pin_ws = 26;   // LRC
    cfg_play.pin_bck = 27;  // BCLK
    cfg_play.pin_data = 25; // DIN

    i2s_play.begin(cfg_play);

    /* //ganancia de salida del audio
    auto cfg_vol = Vol_play.defaultConfig();
    cfg_vol.sample_rate = 44100;
    cfg_vol.bits_per_sample = 16;
    cfg_vol.channels = 1;
    cfg_vol.allow_boost = true; //permite aumentar el volumen por encima de 1.0
    Vol_play.begin(cfg_vol);
    Vol_play.setVolume(0.5); */
}
