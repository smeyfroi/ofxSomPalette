#pragma once

#include "ofMain.h"
#include "ofxAudioAnalysisClient.h"
#include "ofxAudioData.h"
#include "ofxSelfOrganizingMap.h"

// The doubles need to be normalised 0.0..1.0
using SomInstanceDataT = std::array<double, 3>;

class SomPalette: public ofThread {

public:
  SomPalette(int width_, int height_, float initialLearningRate, int numIterations);
  ~SomPalette();
  bool isIterating() { return som.getCurrentIteration() < som.getNumIterations(); }
  void addInstanceData(SomInstanceDataT instanceData);
  void update(); // move pixels into a GL texture on main thread
  bool keyPressed(int key);
  void draw();
  ofColor getColorAt(int x, int y) const;
  ofColor getColor(int i) { return palette[i]; }
  bool isVisible = false;

protected:
  void threadedFunction() override;

private:
  int width, height;
  ofxSelfOrganizingMap som;

  ofThreadChannel<SomInstanceDataT> newInstanceData;
  ofThreadChannel<ofPixels> newPalettePixels;
  bool isNewPalettePixelsReady;

  ofTexture paletteTexture; // GL texture for the palette
  
  // Fixed as an 8-color palette
  std::array<ofColor, 8> palette;
  
  void updatePalette(const ofPixels& pixels);
};
