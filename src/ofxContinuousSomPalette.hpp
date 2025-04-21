#pragma once

#include "ofxSomPalette.h"

// Maintains a number of palettes that renew over time
// to avoid settling into one fixed palette
class ContinuousSomPalette {
public:
  ContinuousSomPalette(int width_=16, int height_=16, float initialLearningRate_=0.01, int numIterations_=5000);
  void addInstanceData(SomInstanceDataT instanceData);
  void update(); // move pixels into a GL texture on main thread
  bool keyPressed(int key);
  void draw();
  ofColor getColor(int i) const;
  bool isVisible() const;
  void setVisible(bool visible_);

private:
  int width, height;
  float initialLearningRate;
  int numIterations;
  bool visible = false;

  std::array<std::unique_ptr<SomPalette>, 2> somPalettePtrs;
  int activeSomPalette;
  int nextActiveSomPalette;
};
