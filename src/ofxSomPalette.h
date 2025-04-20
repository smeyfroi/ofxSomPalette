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
  void addInstanceData(SomInstanceDataT& instanceData);
  void update(); // move pixels into a GL texture on main thread
  bool keyPressed(int key);
  void draw();
  ofColor getColorAt(int x, int y) const;

protected:
  void threadedFunction() override;

private:
  int width, height;
  ofxSelfOrganizingMap som;

  ofThreadChannel<SomInstanceDataT> newInstanceData;
  ofThreadChannel<ofPixels> newPalettePixels;
  bool isNewPalettePixelsReady;
  ofPixels pixels; // non-GL pixels for the palette

  ofTexture paletteTexture; // GL texture for the palette
};
