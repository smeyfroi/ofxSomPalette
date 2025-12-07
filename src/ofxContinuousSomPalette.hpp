#pragma once

#include <array>
#include <memory>

#include "ofxSomPalette.h"

// Maintains a number of palettes that renew over time
// to avoid settling into one fixed palette
class ContinuousSomPalette {
public:
  ContinuousSomPalette(int width_=16, int height_=16, float initialLearningRate_=0.015, int numIterations_=4000);
  void addInstanceData(SomInstanceDataT instanceData);
  void update(); // move pixels into a GL texture on main thread
  void switchPalette();
  bool nextPaletteIsReady() const;
  bool keyPressed(int key);
  void draw();
  ofColor getColor(int i) const;
  bool isVisible() const;
  void setVisible(bool visible_);
  const ofFloatPixels& getPixelsRef() const;
  const ofTexture* getActiveTexturePtr() const;
  const ofTexture* getNextTexturePtr() const;
  void setNumIterations(int numIterations); // propogate to underlying somPalettes

  int width, height;
  float initialLearningRate;
  int numIterations;

private:
  bool visible = false;

  std::array<std::unique_ptr<SomPalette>, 2> somPalettePtrs;
  int activeSomPalette;
  int nextActiveSomPalette;
  
  constexpr static float startNextPaletteAt = 0.5; // start next palette when current palette is halfway through its iterations
  constexpr static float paletteReadyAt = 0.2; // next palette is ready when it is 20% through its iterations
};
