#include <Audio.h>
#include <FastLED.h>
#include <deque>
#include <AudioStream.h>
#include <Arduino.h>

#define NUM_LEDS   60
#define NUM_LEDS1  180 
#define LED_PIN0   25
#define CLOCK_PIN  27
#define LED_PIN1   33
#define CLOCK_PIN1  31

#define MODE_INTERVAL 5000
#define BPM_INTERVAL 1500

//PULSE
#define SCROLL_SPEED 1
unsigned long lastUpdate;

CRGB ledArray[NUM_LEDS];
CRGB ledArray1[NUM_LEDS1]; // Create a separate array for ledStrip1

CRGB tempLeds[NUM_LEDS];

//const int myInput = AUDIO_INPUT_MIC;
const int myInput = AUDIO_INPUT_LINEIN;
AudioInputI2S          audioInput;
AudioMixer4            mixer;
AudioAnalyzeFFT1024    fft;

// AudioConnection myConnection(source, sourcePort, destination, destinationPort);
AudioConnection patchCord1(audioInput, 0, mixer, 0);
AudioConnection patchCord2(audioInput, 1, mixer, 1);
AudioConnection patchCord3(mixer, fft);              // using mixer for stereo line-in connection

//AudioConnection patchCord1(sinewave, 0, myFFT, 0);
AudioControlSGTL5000 audioShield;

const unsigned int bufferSize = 250;                         //related to deque container

  float bassRange = 0, allRange = 0;
  float maxBassAmp = 0, maxAllAmp = 0;
  float avgBassKick;

float bassAvg, allAvg;
int bassAvgCnt = 0, allAvgCnt;
float bassAvgOut = 0, allAvgOut = 0;

int bassBin = 0, allBin = 0;

int modeDirection = 0;
int modeReaction = 0;

float maxBassKick;
int maxBassBin;

int allBrigh;

unsigned int maxBrigh0 = 250;
unsigned int maxBrigh1 = 250;

const int minBrigh = 30;
int fadeBrightnessBy = maxBrigh0 / 38;
int fadeBy = 3;

unsigned int colHue1 = 0;
unsigned int colHue2 = 0;
int newHue;
float prevAmp;
int colorCombo = 0;
int rndTime;

          int fallingstarBrigh = maxBrigh0;
          int brightnessLowering = maxBrigh0 / 5;

          int fadeBrightness;

unsigned long currentMillis = millis();
unsigned long previousMillis = 0;

unsigned long currentMillisMode = millis();
unsigned long previousMillisMode = 0;
unsigned long lastBassKick = 0;

int hue0, brightness0, hue1, brightness1;
int mode;
int bytesReceived = 0;
byte messageBuffer[8];  // Increased size to 8 to handle mode messages

void setup() {
  FastLED.addLeds<APA102, LED_PIN0, CLOCK_PIN,  BGR>(ledArray, NUM_LEDS);
  FastLED.addLeds<APA102, LED_PIN1, CLOCK_PIN1,  BGR>(ledArray1, NUM_LEDS1);
  FastLED.setBrightness(220);

  AudioMemory(30);
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.lineInLevel(7);
  //audioShield.micGain(15);
  fft.windowFunction(AudioWindowBlackman1024);
  Serial.begin(115200);
  Serial1.begin(115200);

  randomSeed(millis());
}

void processMessage() {
    if (messageBuffer[0] == 0xFF) {  // Check if it's a data message
        hue0 = (messageBuffer[1] << 8) | messageBuffer[2];
        brightness0 = messageBuffer[3];
        hue1 = (messageBuffer[4] << 8) | messageBuffer[5];
        brightness1 = messageBuffer[6];

        Serial.print("Hue0: "); Serial.println(hue0);
        Serial.print("Brightness0: "); Serial.println(brightness0);
        Serial.print("Hue1: "); Serial.println(hue1);
        Serial.print("Brightness1: "); Serial.println(brightness1);
        
        colHue1 = hue0;
        maxBrigh0 = brightness0;
        colHue2 = hue1;
        maxBrigh1 = brightness1;
        
    } else if (messageBuffer[0] == 0xFE) {  // Check if it's a mode message
        mode = messageBuffer[1];
        
        Serial.print("Mode: "); Serial.println(mode);
        modeReaction = mode;
    }
    bytesReceived = 0;  // Reset the counter after processing
}

class LEDController {
private:
  int numLeds;
  CRGB* leds;
  
public:
  LEDController(int initialNumLeds, CRGB* initialLeds)
    : numLeds(initialNumLeds), leds(initialLeds)
  {
    
  }

  void moveLeds(int color, int brightness)  {  
    int firstLED = 0;
    int lastLED = numLeds - 1;
    int middleLED = numLeds / 2 - 1;
    
    switch (modeDirection)  {
      case 0:   //move leds outside from middle
        leds[middleLED] = CHSV(color, 255, brightness);
        // Shift LEDs from the middle to the right
        for (int i = numLeds - 1; i > middleLED; i--)
            leds[i] = leds[i - 1];
      
        // Shift LEDs from the middle to the left
        for (int j = 0; j < middleLED; j++)
            leds[j] = leds[j + 1];
      break;
      
    case 1:   //move leds inside from both sides 
        for (int i = 0; i < numLeds; i++) {   // Copy the current state to the temporary buffer
            tempLeds[i] = leds[i];
        }
    
        // Set the first and last LEDs in the temporary buffer
        tempLeds[firstLED] = CHSV(color, 255, brightness);
        tempLeds[lastLED] = CHSV(color, 255, brightness);
    
        // Calculate the new positions and update the temporary buffer
        for (int i = firstLED + 1, j = lastLED - 1; i < middleLED && j >= middleLED; i++, j--) {
            tempLeds[i] = leds[i - 1];
            tempLeds[j] = leds[j + 1];
        }
    
        // Copy the updated state back to the actual LED array
        for (int i = 0; i < numLeds; i++) {
            leds[i] = tempLeds[i];
        }
    break;

      case 2:
        leds[firstLED] = CHSV(color, 255, brightness);
        for (int i = numLeds - 1; i > 0; i--)
            leds[i] = leds[i - 1];
    break;

      case 3:
        leds[lastLED] = CHSV(color, 255, brightness);
        for (int j = 0; j < numLeds - 1; j++)
            leds[j] = leds[j + 1];
    break;

      case 4: //STROBE
          leds[firstLED] = CHSV(color, 255, brightness);
          leds[lastLED] = CHSV(color, 255, brightness);
      
          // Shift LEDs from the right towards the middle
          for (int i = lastLED - 1; i > middleLED; i--)
              leds[i] = leds[i + 1];
      
          // Shift LEDs from the left towards the middle
          for (int j = firstLED + 1; j < middleLED; j++)
              leds[j] = leds[j - 1];
     break;
    }
  }

  void updateLEDs(float amplitudePerRange, float avgAmplitudePerRange, float maxAmplitudePerBin, int& brightnessFadeBy) {
      /*switch(colorCombo)  {
        case 0:
          colHue1 = 115;
          colHue2 = 255;
      break;

        case 1:
          colHue1 = 75;
          colHue2 = 240;
      break;

        case 2:
          colHue1 = 195;
          colHue2 = 25;
      break;

        case 3:
          colHue1 = 195;
          colHue2 = 115;
      break;

        case 4:
          colHue1 = 240;
          colHue2 = 80;
          break;

        case 5:
          colHue1 = 25;
          colHue2 = 225;
      break;
      }*/

      switch(modeReaction)  {
        case 2:
          if (maxAmplitudePerBin > maxBassKick * 0.7 && maxAmplitudePerBin > avgBassKick * 0.7 && maxAmplitudePerBin >= 0.01) {
          lastBassKick = millis();
          moveLeds(colHue1, maxBrigh0);
          } else {
            if (allRange >= allAvgOut * 1.2)  {
              allBrigh += fadeBrightnessBy;
            } else {
              allBrigh -= fadeBrightnessBy;
            }
            if (allBrigh <= minBrigh) {
              allBrigh = minBrigh;
            }
            else if (allBrigh >= maxBrigh1) {
              allBrigh  = maxBrigh1;
            }
              moveLeds(colHue2, allBrigh);
          }
        break;

        case 3:
          if (maxAmplitudePerBin > maxBassKick * 0.7 && maxAmplitudePerBin > avgBassKick * 0.7 && maxAmplitudePerBin >= 0.01) {
            lastBassKick = millis();
            fadeBrightness = maxBrigh0;
            for (int i = 0; i < numLeds; i++) {
              this->leds[i] = CHSV(colHue1, 255, fadeBrightness);
            }
          } else {
            fadeBrightness -= fadeBy;
            if (fadeBrightness <= minBrigh) {
              fadeBrightness = 0;
            }
            for (int i = 0; i < numLeds; i++) {
              this->leds[i] = CHSV(colHue1, 255, fadeBrightness);
            }
          }
      break;

        case 0:
          if (maxAmplitudePerBin > maxBassKick * 0.7 && maxAmplitudePerBin > avgBassKick * 0.7 && maxAmplitudePerBin >= 0.01) {
            lastBassKick = millis();
            fallingstarBrigh = maxBrigh0;
            int newHue = colHue1;
            moveLeds(colHue1, fallingstarBrigh);
          } else {
            newHue -= 1;
            fallingstarBrigh -= fadeBrightnessBy;
            if (fallingstarBrigh >= minBrigh) {
              moveLeds(newHue, fallingstarBrigh);
            } else {
              newHue = colHue1;
              moveLeds(colHue2, 0);
            }
          }
      break;
        
        /*case 3:
          int fallingstarFlag = 0;
          if (maxAmplitudePerBin > maxBassKick * 0.75 && maxAmplitudePerBin > avgBassKick * 0.75 && maxAmplitudePerBin >= 0.02) {
            fallingstarBrigh = maxBrigh;
            moveLeds(colHue1, fallingstarBrigh);
          } else {
             fallingstarBrigh -= fadeBrightnessBy;
            if (fallingstarBrigh >= minBrigh) {
              fallingstarFlag = 1;
          } 

          switch(fallingstarFlag) {
            case 0:
              moveLeds(colHue1, fallingstarBrigh);
          break;
          }         
        }
        break;*/
          
      case 1:
        if (maxAmplitudePerBin > maxBassKick * 0.7 && maxAmplitudePerBin > avgBassKick * 0.7 && maxAmplitudePerBin >= 0.01) {
          lastBassKick = millis();
          moveLeds(colHue2, maxBrigh1);
        } else {
          if (allRange >= allAvgOut * 1.2)  {
            allBrigh += fadeBrightnessBy;
          } else {
            allBrigh -= fadeBrightnessBy;
          }
          if (allBrigh <= minBrigh) {
            allBrigh = minBrigh;
          }
          else if (allBrigh >= maxBrigh1) {
            allBrigh  = maxBrigh1;
          }
            moveLeds(colHue2, allBrigh);
        }
    break;  
    }
  }

  void fadeDown(int& brightness) {
    if (brightness >= fadeBrightnessBy * 2 && brightness > 20) {
      brightness = brightness - fadeBrightnessBy;

      for (int i = 0; i < numLeds; i++ ) {
        this->leds[i].fadeLightBy(-brightness+255);
      }
    } else {
      brightness = 0;
      this->setBlack();
    }
  }
    void setBlack() {
    for (int i = 0; i < numLeds; i++) {
      this->leds[i] = CRGB::Black;
    }
  }
};

class FFTBuffer {
private:
  const unsigned int arraySize = 250;
  int avgOut;
  std::deque<float> fftBuffer;            // Added deque for buffer
  int arrSz;

public:
  FFTBuffer(int initialBufferSize)
    : arraySize(initialBufferSize),
      fftBuffer(initialBufferSize) 
  {
    arrSz = arraySize;
  }

  float movingAverage(const std::deque<float>& fftBuffer) {
    float sum = 0;
    for (float Ampue : fftBuffer) {
      sum += Ampue;
    }
    return sum / arrSz;
  }

  void updateBuffer(float newestBufferAmp, float& avgOut) {
    fftBuffer.pop_front();
    fftBuffer.push_back(newestBufferAmp);
    if (fftBuffer.size() >= arrSz) {
      avgOut = movingAverage(fftBuffer);
    }
  }
 
};

    LEDController ledStrip(NUM_LEDS, ledArray);
    LEDController ledStrip1(NUM_LEDS1, ledArray1);

    FFTBuffer subbassFreqBuffer(bufferSize);
    FFTBuffer allFreqBuffer(bufferSize);
    FFTBuffer maxBassKickBuff(bufferSize);

void FFTnLeds() {
  float amp = 0;
  int bin = 0;
  
  if (fft.available()) {
      bassRange = 0, allRange = 0;
      maxBassAmp = 0;
      bassBin = 0;
  
      
      for (bin = 0; bin < 300; bin++) {
        amp = fft.read(bin);
  
        if (bin >= 4 && bin <= 6) {
          if (amp >= maxBassAmp) {
            maxBassAmp = amp;
            bassBin = bin;
          }
          if (maxBassAmp > maxBassKick) {
            maxBassKick = maxBassAmp;
            maxBassBin = bassBin;
          } else  {
            maxBassKick -= 0.00001;
          }
          if (amp * 0.5 >= prevAmp || amp * 1.5 <= prevAmp) {
            bassRange += amp;
          }
          prevAmp = amp;
        }
        else if (bin >= 0 && bin <= 300)  {
          allRange += amp;
        }
      }
  
      subbassFreqBuffer.updateBuffer(bassRange, bassAvgOut);
      allFreqBuffer.updateBuffer(allRange, allAvgOut);
      maxBassKickBuff.updateBuffer(maxBassKick, avgBassKick);
      
      ledStrip.updateLEDs(bassRange, bassAvgOut, maxBassAmp, fadeBy);
      ledStrip1.updateLEDs(bassRange, bassAvgOut, maxBassAmp, fadeBy);
      FastLED.show();

      /*Serial.print(bassRange);
      Serial.print(", ");
      Serial.print(allRange);
      Serial.println(", ");*/

      Serial.print(maxBassKick);
      Serial.print(", ");
      Serial.print(avgBassKick);
      Serial.print(", ");
      Serial.print(bassRange);
      Serial.println(", ");
    }
  }

    unsigned long generateRndTime(int minimalTimeGenerated, int maximumTimeGenerated) {
    if (minimalTimeGenerated >= maximumTimeGenerated) {
      // Handle invalid input where the minimum is not less than the maximum
      return minimalTimeGenerated;
    }
  
    // Generate a random time duration within the specified range
    return random(minimalTimeGenerated, maximumTimeGenerated + 1);
  }

  void changeMode() {
    if (currentMillis - previousMillis >= rndTime) {
      previousMillis = currentMillis;
      /*modeDirection++;
      if (modeDirection >= 2)  {
        modeDirection = 0;
      }*/
      colorCombo++;
      rndTime = generateRndTime(5000, 5001);
        if (colorCombo > 5)  {
        colorCombo = 0;
        modeReaction++;
      }
      //modeReaction++;
        if (modeReaction >= 3) {
          modeReaction = 0;
        }
    }
  }

  void changeModeAfter1500ms() {
      currentMillisMode = millis();
      //Serial.println("Checking mode change..."); // Diagnostic print
  
      if (currentMillisMode - previousMillisMode >= BPM_INTERVAL) {
          //Serial.println("Interval elapsed."); // Diagnostic print
  
          if (currentMillisMode - lastBassKick >= BPM_INTERVAL) {
              //Serial.println("Changing mode."); // Diagnostic print
              previousMillisMode = currentMillisMode;
              modeReaction++;
              //Serial.print("Mode changed to: "); Serial.println(modeReaction);
              if (modeReaction >= 4) {
                  modeReaction = 0;
              }
          }
      }
  }




void loop() { 
  currentMillis = millis();
  
  if (Serial1.available() > 0) {
      byte incomingByte = Serial1.read();
      // Check for message header
      if ((bytesReceived == 0 && (incomingByte == 0xFF || incomingByte == 0xFE)) || bytesReceived > 0) {
          messageBuffer[bytesReceived++] = incomingByte;
      }

      // If we've received a full message, process it
      if ((messageBuffer[0] == 0xFF && bytesReceived == 7) || (messageBuffer[0] == 0xFE && bytesReceived == 2)) {
          processMessage();
      }

      // If the buffer is somehow overrun, or an invalid header is received, reset to start fresh
      if (bytesReceived >= sizeof(messageBuffer) || (bytesReceived == 1 && messageBuffer[0] != 0xFF && messageBuffer[0] != 0xFE)) {
          bytesReceived = 0;
      }
  }
  //changeMode();
  //changeModeAfter1500ms();
  FFTnLeds();
}
