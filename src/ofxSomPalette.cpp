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

// Sample 8 colors from the SOM pixels round the edges
// X..X..X
// .......
// X.....X
// .......
// X..X..X
void SomPalette::updatePalette(const ofPixels& pixels) {
  palette[0] = pixels.getColor(0, 0);
  palette[1] = pixels.getColor(pixels.getWidth()/2, 0);
  palette[2] = pixels.getColor(pixels.getWidth()-1, 0);
  palette[3] = pixels.getColor(0, pixels.getHeight()/2);
  palette[4] = pixels.getColor(pixels.getWidth()-1, pixels.getHeight()/2);
  palette[5] = pixels.getColor(0, pixels.getHeight()-1);
  palette[6] = pixels.getColor(pixels.getWidth()/2, pixels.getHeight()-1);
  palette[7] = pixels.getColor(pixels.getWidth()-1, pixels.getHeight()-1);
  std::sort(palette.begin(),
            palette.end(),
            [](ofColor a, ofColor b){
              return a.getLightness() < b.getLightness();
            });
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
    updatePalette(pixels);
  }
}

bool SomPalette::keyPressed(int key) {
  std::string timestamp = ofGetTimestampString();
  if (key == 'S' && paletteTexture.isAllocated()) {
    ofPixels p;
    paletteTexture.readToPixels(p);
    ofSaveImage(p, ofFilePath::getUserHomeDir()+"/Documents/som/"+timestamp+"-snapshot.png", OF_IMAGE_QUALITY_BEST);
    ofFbo fbo;
    fbo.allocate(8 * 64, 64, GL_RGB8);
    fbo.begin();
    ofFill();
    for (int i = 0; i < palette.size(); i++) {
      ofSetColor(getColor(i));
      ofDrawRectangle(i*64, 0.0, 64, 64);
    }
    fbo.end();
    fbo.readToPixels(p);
    ofSaveImage(p, ofFilePath::getUserHomeDir()+"/Documents/som/"+timestamp+"-palette.png", OF_IMAGE_QUALITY_BEST);
    return true;
  }
  return false;
}

void SomPalette::draw() {
  ofPushStyle();
  if (paletteTexture.isAllocated()) {
    paletteTexture.draw(0, 0, ofGetWindowWidth(), ofGetWindowHeight());
  }
  float chipWidth = ofGetWindowWidth() / palette.size();
  ofFill();
  for (int i = 0; i < palette.size(); i++) {
    ofSetColor(getColor(i));
    ofDrawRectangle(i*chipWidth, 0.0, chipWidth, chipWidth);
  }
  ofPopStyle();
}

ofColor SomPalette::getColorAt(int x, int y) const {
  ofPixels pixels;
  paletteTexture.readToPixels(pixels);
  return pixels.getColor(x, y);
}
