#include "ofxSomPalette.h"

SomPalette::SomPalette(int width_, int height_, float initialLearningRate_, int numIterations_) :
width { width_ },
height { height_ }
{
  setThreadName("SomPalette");

  som.setInitialLearningRate(initialLearningRate_);
  som.setNumIterations(numIterations_);

  double minInstance[3] = { 0, 0, 0 };
  double maxInstance[3] = { 1.0, 1.0, 1.0 };
  som.setFeaturesRange(3, minInstance, maxInstance);
  som.setMapSize(width, height); // can go to 3 dimensions

  som.setup();

  startThread();
}

SomPalette::~SomPalette() {
  newInstanceData.close();
  waitForThread(true);
}

void SomPalette::addInstanceData(SomInstanceDataT instanceData) {
  if (isIterating()) newInstanceData.send(instanceData);
}

// TODO: Make sure we can't be overwhelmed if producer fills queue faster than we consume (e.g. could just do the SOM not the pixels)
void SomPalette::threadedFunction() {
  SomInstanceDataT instanceData;
  ofPixels p;
  p.allocate(width, height, OF_IMAGE_COLOR);

  while (newInstanceData.receive(instanceData)) {
    som.updateMap(instanceData.data());
    for (int i = 0; i < width; i++) {
      for (int j = 0; j < height; j++) {
        double* c = som.getMapAt(i, j);
        ofFloatColor col(c[0], c[1], c[2]);
        p.setColor(i, j, col);
      }
    }
//    newPalettePixels.send(std::move(p));
    newPalettePixels.send(p);
  }
}

void SomPalette::update() {
  ofPixels pixels;
  isNewPalettePixelsReady = false;
  while (newPalettePixels.tryReceive(pixels)) {
    isNewPalettePixelsReady = true;
  }
  if (isNewPalettePixelsReady) {
    if (!paletteTexture.isAllocated()) paletteTexture.allocate(pixels);
    paletteTexture.loadData(pixels);
  }
}

bool SomPalette::keyPressed(int key) {
  if (key == 'S' && paletteTexture.isAllocated()) {
    ofPixels p;
    paletteTexture.readToPixels(p);
    ofSaveImage(p, ofFilePath::getUserHomeDir()+"/Documents/som/snapshot-"+ofGetTimestampString()+".png", OF_IMAGE_QUALITY_BEST);
    return true;
  }
  return false;
}

void SomPalette::draw() {
  if (paletteTexture.isAllocated()) {
    paletteTexture.draw(0, 0, ofGetWindowWidth(), ofGetWindowHeight());
  }
}

ofColor SomPalette::getColorAt(int x, int y) const {
  ofPixels pixels;
  paletteTexture.readToPixels(pixels);
  return pixels.getColor(x, y);
}
