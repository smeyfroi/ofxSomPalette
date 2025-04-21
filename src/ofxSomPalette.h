#pragma once

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
  bool isIterating() { return som.getCurrentIteration() < som.getNumIterations(); }
  void addInstanceData(SomInstanceDataT instanceData);
  void update(); // move pixels into a GL texture on main thread
  bool keyPressed(int key);
  void draw(bool forceVisible = false);
  ofColor getColorAt(int x, int y) const;
  ofColor getColor(int i) const { return palette[i]; }
  bool isVisible() const { return visible; };
  void setVisible(bool visible_) { visible = visible_; };
  int getCurrentIteration() { return som.getCurrentIteration(); };
  int getNumIterations() { return som.getNumIterations(); };

protected:
  void threadedFunction() override;

private:
  int width, height;
  float initialLearningRate;
  int numIterations;

  ofxSelfOrganizingMap som;

  ofThreadChannel<SomInstanceDataT> newInstanceData;
  ofThreadChannel<ofPixels> newPalettePixels;
  bool isNewPalettePixelsReady;

  ofTexture paletteTexture; // GL texture for the palette
  
  // Fixed as an 8-color palette
  std::array<ofColor, 8> palette;
  
  void updatePalette(const ofPixels& pixels);
  
  bool visible = false;
};
