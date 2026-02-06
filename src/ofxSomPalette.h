#pragma once

#include <array>
#include <atomic>

#include "ofMain.h"
#include "ofxSelfOrganizingMap.h"

// The doubles need to be normalised 0.0..1.0
using SomInstanceDataT = std::array<double, 3>;

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
  // grayGain: centroid -> brightness contribution
  // chromaGain: crest/zcr -> chroma contribution
  void setColorizerGains(float grayGain, float chromaGain);
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
  std::atomic<float> warmStartMix { 0.60f };
  bool shouldWarmStartOnNextInstance { true };
  
  void updatePalette();
  
  bool visible = false;
};
