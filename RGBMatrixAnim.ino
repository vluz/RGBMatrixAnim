#include <Adafruit_NeoPixel.h>

#define LED_PIN A0
#define LED_COUNT 60
#define BRIGHTNESS 10

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Animation variables
unsigned long prevAnimTime = 0;
const long animInterval = 15000;  // 15 seconds per animation
uint8_t currentAnim = 0;
const uint8_t numAnimations = 11;  // 11 animations

// Animation-specific variables
uint16_t firstPixelHue = 0;    // For rainbowCycle
uint32_t solidColor = 0;       // For solidColorFade
uint16_t wipePosition = 0;     // For colorWipe
uint8_t chasePosition = 0;     // For theaterChase
uint8_t hue = 0;               // For solidColorFade
uint8_t pulseValue = 128;      // For pulseEffect - mid brightness
uint8_t pulseDirection = 1;    // For pulseEffect
uint16_t runnerPosition = 0;   // For runningLights
int8_t runnerDirection = 1;    // For runningLights
bool reverseFire = false;      // Fire direction
uint8_t bitmapHue = 0;         // For bitmapColorCycle
float waterLevels[6] = { 0 };  // Water heights per column
float velocities[6] = { 0 };   // Physics velocities
const float baseLevel = 4.0;   // Base water level (40% of 10 rows)
const float damping = 0.8;     // Wave energy reduction
const float spread = 0.26;     // Wave spread between columns

// Color extraction functions
uint8_t getRed(uint32_t color) {
  return (color >> 16) & 0xFF;
}

uint8_t getGreen(uint32_t color) {
  return (color >> 8) & 0xFF;
}

uint8_t getBlue(uint32_t color) {
  return color & 0xFF;
}

// Setup
void setup() {
  strip.begin();
  strip.show();
  strip.setBrightness(BRIGHTNESS);
  randomSeed(analogRead(0));  // Seed for random colors
}

// Main loop
void loop() {
  unsigned long currentMillis = millis();
  // Switch animation every animInterval
  if (currentMillis - prevAnimTime >= animInterval) {
    prevAnimTime = currentMillis;
    currentAnim = (currentAnim + 1) % numAnimations;
    resetAnimationState();
  }
  // Run current animation
  switch (currentAnim) {
    case 0: rainbowCycle(); break;
    case 1: fireEffect(); break;
    case 2: colorWipe(); break;
    case 3: theaterChase(); break;
    case 4: pulseEffect(); break;
    case 5: runningLights(); break;
    case 6: patternColorCycle(); break;
    case 7: colorRandom(); break;
    case 8: solidColorFade(); break;
    case 9: waterSloshEffect(); break;
    case 10: bitmapColorCycle(); break;
  }
  delay(20);
}

// Rest animations states and randomizes colors
void resetAnimationState() {
  // Reset all animation variables
  firstPixelHue = random(0, 65535);
  solidColor = 0;
  wipePosition = 0;
  chasePosition = 0;
  hue = random(0, 65535);
  pulseValue = 128;  // Start at mid brightness for visible pulse
  pulseDirection = 1;
  runnerPosition = 0;
  runnerDirection = 1;
  bitmapHue = random(0, 65535);
  for (int i = 0; i < 6; i++) {
    waterLevels[i] = baseLevel;  // Set to base water level
    velocities[i] = 0;
  }
  strip.clear();
}

// Animation 1: Smooth moving rainbow
void rainbowCycle() {
  for (int i = 0; i < strip.numPixels(); i++) {
    uint16_t pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
  }
  strip.show();
  firstPixelHue += 256;  // Rainbow movement speed
}

// Animation 2: Smooth color fading
void solidColorFade() {
  solidColor = strip.gamma32(strip.ColorHSV(hue * 256, 255, 255));
  strip.fill(solidColor);
  strip.show();
  hue++;  // Increment hue for next color
}

// Animation 3: Color wipe effect
void colorWipe() {
  strip.setPixelColor(wipePosition, strip.gamma32(strip.ColorHSV(random(0, 65535), 255, 255)));
  strip.show();
  wipePosition = (wipePosition + 1) % strip.numPixels();
}

// Animation 4: Theater chase pattern
void theaterChase() {
  strip.clear();
  // Draw dots at intervals
  for (int i = 0; i < strip.numPixels(); i += 4) {
    int pos = (i + chasePosition) % strip.numPixels();
    strip.setPixelColor(pos, strip.gamma32(strip.ColorHSV(i * 1024, 255, 255)));
  }
  strip.show();
  chasePosition = (chasePosition + 1) % 4;  // Move chase pattern
}

// Animation 5: Breathing Pulse Effect
void pulseEffect() {
  // Calculate color (slowly changing hue)
  uint32_t color = strip.gamma32(strip.ColorHSV(hue * 256, 255, 255));
  // Apply pulse brightness to the color
  uint8_t r = (getRed(color) * pulseValue) / 255;
  uint8_t g = (getGreen(color) * pulseValue) / 255;
  uint8_t b = (getBlue(color) * pulseValue) / 255;
  strip.fill(strip.Color(r, g, b));
  strip.show();
  // Update pulse value with easing
  pulseValue += pulseDirection * 5;
  // Change direction at extremes
  if (pulseValue >= 250 || pulseValue <= 5) {
    pulseDirection *= -1;
    if (pulseValue <= 5) hue += random(0, 65535);  // Only change color at bottom of pulse
  }
}

// Animation 6: Running Lights
void runningLights() {
  strip.clear();
  // Create a group of lit LEDs
  for (int i = 0; i < 4; i++) {
    int pos = (runnerPosition + i) % strip.numPixels();
    strip.setPixelColor(pos, strip.gamma32(strip.ColorHSV(hue * 256, 255, 255)));
  }
  strip.show();
  // Move runner position
  runnerPosition += runnerDirection;
  // Reverse direction at ends
  if (runnerPosition >= strip.numPixels() - 4 || runnerPosition <= 0) {
    runnerDirection *= -1;
    hue += 2000;  // Change color on bounce
  }
}

// Animation 7: Fire Effect
void fireEffect() {
  static uint8_t heat[10][6] = { 0 };  // Heat buffer (10 rows, 6 cols)
  // 1. Generate new heat at base
  for (uint8_t c = 0; c < 6; c++) {
    // Sustained base heat with flicker
    heat[0][c] = random(30, 220);
    // Random sparks (hotter flames)
    if (random(100) < 15) {
      heat[0][c] = 255;  // Max heat spark
    }
  }
  // Propagate heat with diffusion physics
  for (uint8_t r = 1; r < 10; r++) {
    for (uint8_t c = 0; c < 6; c++) {
      // Get neighboring cells (wrap horizontally)
      uint8_t left = heat[r - 1][(c + 5) % 6];
      uint8_t center = heat[r - 1][c];
      uint8_t right = heat[r - 1][(c + 1) % 6];
      // Diffuse heat from below (weighted average)
      uint16_t newHeat =
        (center * 4) +  // Main heat source (60%)
        (left * 3) +    // Left diffusion (20%)
        (right * 3);    // Right diffusion (20%)
      // Apply cooling and random flicker
      newHeat = (newHeat * 11) / 100;  // 15% cooling
      newHeat = constrain(newHeat - random(20, 150), 0, 255);
      heat[r][c] = newHeat;
    }
  }
  // Render heat to LEDs with fire color palette
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    // Convert to matrix coordinates
    uint16_t physIdx = reverseFire ? (59 - i) : i;
    uint8_t row = 9 - (physIdx / 6);  // 0=bottom, 9=top
    uint8_t col = physIdx % 6;
    uint8_t temp = heat[row][col];
    uint8_t r, g, b;
    // Fire color gradient (black -> red -> orange -> yellow)
    if (temp < 60) {
      r = 0;
      g = 0;
      b = 0;  // Cooled embers
    } else if (temp < 120) {
      r = temp;
      g = temp / 8;
      b = 0;  // Deep red
    } else if (temp < 200) {
      r = 255;
      g = (temp * 3) / 4;  // Orange transition
      b = 0;
    } else {
      r = 255;
      g = 60 + (temp - 180) / 3;  // Yellow highlights
      b = (temp - 180) / 4;
    }
    strip.setPixelColor(i, r, g, b);
  }
  strip.show();
  delay(20);
}

// Animation 7: Random pixels
void colorRandom() {
  uint32_t firstPixelHue = 0;
  for (int i = 0; i < strip.numPixels(); i++) {  // For each pixel in strip...
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(random(0, 65535), random(0, 255), random(0, 255))));
  }
  strip.show();  // Update strip with new contents
  delay(random(30, 300));
}

// Animation 8: Bitmap with color cycling
void bitmapColorCycle() {
  // 6x10 bitmap here (1=on, 0=off)
  // Pattern should be inverted
  static const bool bitmap[10][6] = {
    { 0, 0, 0, 0, 0, 0 },  // Row 0
    { 0, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0 },
    { 0, 1, 0, 0, 1, 0 },
    { 1, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 1 },
    { 1, 0, 1, 1, 0, 1 },
    { 0, 1, 0, 0, 1, 0 },
    { 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0 }  // Row 9
  };
  // Generate current color from hue
  uint32_t color = strip.gamma32(strip.ColorHSV(bitmapHue * 256, 255, 255));
  // Render bitmap to matrix
  for (uint8_t row = 0; row < 10; row++) {
    for (uint8_t col = 0; col < 6; col++) {
      // Calculate LED index (accounting for matrix layout)
      uint16_t index = (9 - row) * 6 + col;
      if (bitmap[row][col]) {
        strip.setPixelColor(index, color);
      } else {
        strip.setPixelColor(index, 0);
      }
    }
  }
  strip.show();
  // Update hue for next cycle (0-255 range covers full spectrum)
  bitmapHue = (bitmapHue + 1) % 256;
}

// Animation 9: pattern
void patternColorCycle() {
  // Define your 6x10 pattern here (1=color, 0=Shifted color)
  // Pattern should be inverted
  static const bool bitmap[10][6] = {
    { 0, 1, 0, 1, 0, 1 },  // Row 0
    { 1, 0, 1, 0, 1, 0 },
    { 0, 1, 0, 1, 0, 1 },
    { 1, 0, 1, 0, 1, 0 },
    { 0, 1, 0, 1, 0, 1 },
    { 1, 0, 1, 0, 1, 0 },
    { 0, 1, 0, 1, 0, 1 },
    { 1, 0, 1, 0, 1, 0 },
    { 0, 1, 0, 1, 0, 1 },
    { 1, 0, 1, 0, 1, 0 }  // Row 9
  };
  // Generate current color from hue
  uint32_t color = strip.gamma32(strip.ColorHSV(bitmapHue * 256, 255, 255));
  // Render bitmap to matrix
  for (uint8_t row = 0; row < 10; row++) {
    for (uint8_t col = 0; col < 6; col++) {
      // Calculate LED index (accounting for matrix layout)
      uint16_t index = (9 - row) * 6 + col;
      if (bitmap[row][col]) {
        strip.setPixelColor(index, color);
      } else {
        strip.setPixelColor(index, (color + 2000));
      }
    }
  }
  strip.show();
  // Update hue for next cycle (0-255 range covers full spectrum)
  bitmapHue = (bitmapHue + 1) % 256;
}

// ANIMATION 10: Water Sloshing Effect
void waterSloshEffect() {
  // Add random splashes periodically
  static unsigned long lastSplash = 0;
  if (millis() - lastSplash > 1400) {
    int col = random(0, 6);
    velocities[col] = random(180, 300) / 100.0;  // Stronger splashes
    lastSplash = millis();
  }
  // Physics simulation for deep water
  for (int col = 0; col < 6; col++) {
    // Calculate neighbor influence with reduced spread
    float left = col > 0 ? waterLevels[col - 1] : baseLevel;
    float right = col < 5 ? waterLevels[col + 1] : baseLevel;
    float spreadForce = (left + right - 2 * waterLevels[col]) * spread;
    // Update physics with stronger restoring force
    velocities[col] += spreadForce - (waterLevels[col] - baseLevel) * 0.15;
    velocities[col] *= damping;
    waterLevels[col] += velocities[col];
  }
  // Render deeper water to LEDs
  for (int col = 0; col < 6; col++) {
    for (int row = 0; row < 10; row++) {
      // Physical LED index (bottom-left origin)
      int index = (9 - row) * 6 + col;
      float depth = waterLevels[col] - row;
      // Water rendering with depth-based color
      if (depth > 2.5) {
        // Deep water (dark blue)
        strip.setPixelColor(index, 0, 0, 100);
      } else if (depth > 1.0) {
        // Medium water (blue)
        strip.setPixelColor(index, 0, 80, 220);
      } else if (depth > 0.1) {
        // Shallow water (light blue)
        strip.setPixelColor(index, 50, 150, 255);
      } else if (depth > 0) {
        // Water surface (foam effect)
        strip.setPixelColor(index, 180, 220, 255);
      } else {
        // Air (off)
        strip.setPixelColor(index, 0);
      }
    }
  }
  strip.show();
}
//END