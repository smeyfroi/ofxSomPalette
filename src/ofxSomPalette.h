#pragma once

#include <array>
#include <atomic>

#include "ofMain.h"
#include "ofxSelfOrganizingMap.h"

// The doubles need to be normalised 0.0..1.0
// Feature layout (per Scheme A colorizer):
//   [0] = centroid  -> hue chroma u axis
//   [1] = crest     -> chroma magnitude (saturation gain)
//   [2] = flatness  -> hue chroma v axis (inverted)
//   [3] = rms       -> lightness
using SomInstanceDataT = std::array<double, 4>;

class SomPalette: public ofThread {

public:
  SomPalette(int width_=16, int height_=16, float initialLearningRate_=0.01, int numIterations_=5000);
  ~SomPalette();
  void setupSom(float initialLearningRate, int numIterations);
  void reset();
  void warmStartFromFirstInstance(float mix = 0.85f);
  bool isIterating() { return som.getCurrentIteration() < som.getNumIterations(); }
  void addInstanceData(SomInstanceDataT instanceData);
  void update(); // move pixels into a GL texture on main thread
  bool keyPressed(int key);

  // Deterministic feature->RGB mapping controls.
  // grayGain: lightness contribution per Scheme A (rms drives lightness)
  // chromaGain: chroma magnitude scale per Scheme A (crest drives saturation gain)
  void setColorizerGains(float grayGain, float chromaGain);

  // Per-channel max for the colorizer output. Useful when downstream layers
  // accumulate and would otherwise saturate to white. Default 1.0 (no cap).
  void setColorizerMaxBrightness(float maxBrightness);

  // Chroma reward in the greedy 8-chip extraction. The score for each candidate cell becomes
  // `minRgbDistance + bias * (max(r,g,b) - min(r,g,b))`. Default 0.0 = old behaviour.
  void setChipSaturationBias(float bias);

  // Per-pixel temporal smoothing applied on the main thread each update(). 0 = no smoothing
  // (snap to latest colorization), larger values mean the texture eases toward new state.
  void setTextureSmoothingSecs(float secs);
  void draw(bool forceVisible = false, bool paletteOnly = false);
  const ofFloatPixels& getPixelsRef() const { return pixels; }
  const ofTexture& getTexture() const { return paletteTexture; }
  ofColor getColorAt(int x, int y) const;
  ofColor getColor(int i) const { return palette[i]; }
  bool isVisible() const { return visible; };
  void setVisible(bool visible_) { visible = visible_; };
  int getCurrentIteration() { return som.getCurrentIteration(); };
  int getNumIterations() { return som.getNumIterations(); };
  void setNumIterations(int numIterations_) { som.setNumIterations(numIterations_); };
  static constexpr size_t size = 8;

protected:
  void threadedFunction() override;

private:
  int width, height;
  float initialLearningRate;
  int numIterations;

  ofxSelfOrganizingMap som;

  ofThreadChannel<SomInstanceDataT> newInstanceData;
  ofThreadChannel<ofFloatPixels> newPalettePixels;
  bool isNewPalettePixelsReady;

  ofFloatPixels pixels; // the pixels that are moved to the GL texture
  ofTexture paletteTexture; // GL texture for the palette
  
  // Fixed as an 8-color palette
  std::array<ofColor, size> palette;

  std::atomic<float> colorizerGrayGain { 1.0f };
  std::atomic<float> colorizerChromaGain { 1.25f };
  std::atomic<float> colorizerMaxBrightness { 1.0f };
  std::atomic<float> chipSaturationBias { 0.0f };
  std::atomic<float> textureSmoothingSecs { 0.0f };
  std::atomic<float> warmStartMix { 0.60f };
  bool shouldWarmStartOnNextInstance { true };
  
  void updatePalette();
  
  bool visible = false;
};
