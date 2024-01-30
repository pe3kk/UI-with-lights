import React, { useEffect, useState } from 'react';
import { Button, StyleSheet, Text, View, TouchableOpacity, ScrollView, TextInput, Platform } from 'react-native';
import { LinearGradient } from 'expo-linear-gradient';
import { SketchPicker } from 'react-color';
import { ColorPicker } from 'react-native-color-picker';
import { Picker } from '@react-native-picker/picker';
import AsyncStorage from '@react-native-async-storage/async-storage';
import { ImageBackground } from 'react-native';

const isWeb = Platform.OS === 'web'; // isWeb = True!

export default function App() {
  const [inputValue, setInputValue] = useState('');
  const [data, setData] = useState(null);
  const [ws, setWs] = useState(null);
  const [color1, setColor1] = useState('#c90076');
  const [color2, setColor2] = useState('#ffd966');
  const [brightness1, setBrightness1] = useState(0);
  const [brightness2, setBrightness2] = useState(0);
  const [backgroundColor, setBackgroundColor] = useState('black');

  useEffect(() => {
    const wsInstance = new WebSocket('ws://158.179.207.196:8080');

    const loadValues = async () => {
      const savedColor1 = await AsyncStorage.getItem('color1');
      const savedColor2 = await AsyncStorage.getItem('color2');
      const savedBrightness1 = await AsyncStorage.getItem('brightness1');
      const savedBrightness2 = await AsyncStorage.getItem('brightness2');
    
      if (savedColor1) setColor1(savedColor1);
      if (savedColor2) setColor2(savedColor2);
      if (savedBrightness1) setBrightness1(JSON.parse(savedBrightness1));
      if (savedBrightness2) setBrightness2(JSON.parse(savedBrightness2));
    };
    
    loadValues();

    wsInstance.onopen = () => {
      if (wsInstance.readyState === WebSocket.OPEN) {
        wsInstance.send('something');
      } else {
        console.log('Cannot send message, WebSocket is not open');
      }
    };

    wsInstance.onmessage = (e) => {
      console.log(e.data);
    };

    wsInstance.onerror = (e) => {
      console.log(e.message);
    };

    wsInstance.onclose = (e) => {
      console.log(e.code, e.reason);
    };

    setWs(wsInstance);

    return () => {
      wsInstance.close();
    };
  }, []);

  const styles = StyleSheet.create({
    container: {
      flex: 1,
      justifyContent: 'center',
      alignItems: 'center',
      backgroundColor: 0,
    },
    button: {
      padding: 15,
      alignItems: 'center',
      borderRadius: 5,
      margin: 10,
    },
    buttonText: {
      color: 'white',
      fontSize: 18,
    },
    picker: {
      width: 50,
    },
    input: {
      color: 'white',
      height: 40,
      borderColor: 'white',
      borderWidth: 1,
      marginTop: 20,
      paddingHorizontal: 10,
    },
  });

const colorPairs = [
  ['#c90076', '#ffd966'],
  ['#f44336', '#f44336'],
  ['#8fce00', '#f44336'],
  ['#8fce00', '#a64d79'],
  ['#cc0000', '#c90076'],
  ['#f44336', '#ffffff'],
];

function hexToHsl(hex) {
  let r = parseInt(hex.slice(1, 3), 16),
      g = parseInt(hex.slice(3, 5), 16),
      b = parseInt(hex.slice(5, 7), 16);
  r /= 255, g /= 255, b /= 255;
  const max = Math.max(r, g, b), min = Math.min(r, g, b);
  let h, s, l = (max + min) / 2;
  if(max === min){
    h = s = 0; // achromatic
  } else {
    const d = max - min;
    s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
    switch(max){
      case r: h = (g - b) / d + (g < b ? 6 : 0); break;
      case g: h = (b - r) / d + 2; break;
      case b: h = (r - g) / d + 4; break;
    }
    h /= 6;
  }
  return { h: h * 255, s: s * 100, l: l * 100 };
}

return (
  <ImageBackground source={require('./assets/background.jpg')} style={styles.container}>
  <ScrollView contentContainerStyle={styles.container}>
    <View style={{ flexDirection: 'row', justifyContent: 'space-around', marginTop: 20 }}>
      <TouchableOpacity
          onPress={() => {
            // Handle COL 1 ONLY button press
            ws && ws.send('0');
          }}
          style={styles.button}
        >
          <Text style={styles.buttonText}>FALLING-STAR</Text>
        </TouchableOpacity>
        <TouchableOpacity
          onPress={() => {
            // Handle COL 1 ONLY button press
            ws && ws.send('1');
          }}
          style={styles.button}
        >
          <Text style={styles.buttonText}>COLOR 1 ONLY</Text>
        </TouchableOpacity>
        <TouchableOpacity
          onPress={() => {
            // Handle COL 1 N 2 button press
            ws && ws.send('2');
          }}
          style={styles.button}
        >
          <Text style={styles.buttonText}>COLOR 1 AND 2</Text>
        </TouchableOpacity>
        <TouchableOpacity
          onPress={() => {
            // Handle on N FadeDown button press
            ws && ws.send('3');
          }}
          style={styles.button}
        >
          <Text style={styles.buttonText}>FLASH AND FADE DOWN</Text>
        </TouchableOpacity>
      </View>

      <Text>COLOR COMBO</Text>
      <View style={{ flexDirection: 'row', alignItems: 'center' }}>
        {isWeb ? (
          <SketchPicker
            color={color1}
            onChangeComplete={color => setColor1(color.hex)}
          />
        ) : (
          <ColorPicker
            onColorSelected={color => setColor1(color)}
            style={{flex: 1}}
          />
        )}
        <Picker
          selectedValue={brightness1}
          onValueChange={(itemValue, itemIndex) => setBrightness1(itemValue)}
          style={styles.picker}
        >
          {[...Array(11).keys()].map((value) => (
            <Picker.Item key={value} label={`${value * 10}%`} value={`${value * 10}`} />
          ))}
        </Picker>
        <TouchableOpacity onPress={async () => {
          const hsl1 = hexToHsl(color1);
          const hsl2 = hexToHsl(color2);
          const brightness1Scaled = Math.round(brightness1 * 2.5);
          const brightness2Scaled = Math.round(brightness2 * 2.5);
          ws && ws.send(`${Math.round(hsl1.h)},${brightness1Scaled},${Math.round(hsl2.h)},${brightness2Scaled},${inputValue}`);

          // Save the values
          try {
            await AsyncStorage.setItem('color1', color1);
            await AsyncStorage.setItem('color2', color2);
            await AsyncStorage.setItem('brightness1', JSON.stringify(brightness1));
            await AsyncStorage.setItem('brightness2', JSON.stringify(brightness2));
          } catch (error) {
            console.error('Failed to save data', error);
          }
        }}>
          <LinearGradient
            colors={[color1, color2]}
            style={styles.button}
          >
            <Text style={styles.buttonText}>COLORS 6</Text>
          </LinearGradient>
        </TouchableOpacity>
        {isWeb ? (
          <SketchPicker
            color={color2}
            onChangeComplete={color => setColor2(color.hex)}
          />
        ) : (
          <ColorPicker
            onColorSelected={color => setColor2(color)}
            style={{flex: 1}}
          />
        )}
        <Picker
          selectedValue={brightness2}
          onValueChange={(itemValue, itemIndex) => setBrightness2(itemValue)}
          style={styles.picker}
        >
          {[...Array(11).keys()].map((value) => (
            <Picker.Item key={value} label={`${value * 10}%`} value={`${value * 10}`} />
          ))}
        </Picker>
      </View>

      <View style={{ flexDirection: 'row', justifyContent: 'space-around', marginTop: 20 }}>
        {colorPairs.map((pair, index) => (
          <TouchableOpacity
            key={index}
            onPress={() => {
              if (index === 5) { // Only apply the changes to button 6 (index 5)
                const hsl1 = hexToHsl(color1);
                const hsl2 = hexToHsl(color2);
                const brightness1Scaled = Math.round(brightness1 * 2.5);
                const brightness2Scaled = Math.round(brightness2 * 2.5);
                ws && ws.send(`${Math.round(hsl1.h)},${brightness1Scaled},${Math.round(hsl2.h)},${brightness2Scaled},${inputValue}`);

                // Save the values
                AsyncStorage.setItem('color1', color1);
                AsyncStorage.setItem('color2', color2);
                AsyncStorage.setItem('brightness1', brightness1);
                AsyncStorage.setItem('brightness2', brightness2);
              } else { // For other buttons, revert to the original code
                setColor1(pair[0]);
                setColor2(pair[1]);
              }
            }}
          >
            <LinearGradient
              colors={pair}
              style={styles.button}
            >
              <Text style={styles.buttonText}>COLORS {index}</Text>
            </LinearGradient>
          </TouchableOpacity>
        ))}
        <TouchableOpacity
          onPress={() => {
            // Change the background color to blue
            setBackgroundColor('blue');
            // Change it back to black after 3 seconds
            setTimeout(() => setBackgroundColor('black'), 3000);
          }}
        >
          <LinearGradient
            colors={['blue', 'blue']}
            style={styles.button}
          >
            <Text style={styles.buttonText}>COLORS 7</Text>
          </LinearGradient>
        </TouchableOpacity>
      </View>
    </ScrollView>
    </ImageBackground>
  );
}
