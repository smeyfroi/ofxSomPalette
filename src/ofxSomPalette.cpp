#include "ofxSomPalette.h"
#include "ofTexture.h"

SomPalette::SomPalette(int width_, int height_, float initialLearningRate_, int numIterations_) :
width { width_ },
height { height_ },
initialLearningRate { initialLearningRate_ },
numIterations { numIterations_ }
{
  setThreadName("SomPalette " + ofToString(this));
  setupSom(initialLearningRate_, numIterations_);
  startThread();
}

SomPalette::~SomPalette() {
  newInstanceData.close();
  newPalettePixels.close();
  waitForThread(true);
}

void SomPalette::setupSom(float initialLearningRate, int numIterations) {
  double minInstance[3] = { 0, 0, 0 };
  double maxInstance[3] = { 1.0, 1.0, 1.0 };
  som.setFeaturesRange(3, minInstance, maxInstance);
  som.setMapSize(width, height); // can go to 3 dimensions
  
  som.setInitialLearningRate(initialLearningRate);
  som.setNumIterations(numIterations);
  som.setup();
}

void SomPalette::reset() {
  som = ofxSelfOrganizingMap();
  setupSom(initialLearningRate, numIterations);
  newInstanceData.clear();
}

void SomPalette::addInstanceData(SomInstanceDataT instanceData) {
  if (isIterating()) newInstanceData.send(instanceData);
}

// TODO: Make sure we can't be overwhelmed if producer fills queue faster than we consume (e.g. could just do the SOM not the pixels)
void SomPalette::threadedFunction() {
  SomInstanceDataT instanceData;
  
  while (newInstanceData.receive(instanceData)) {
    som.updateMap(instanceData.data());
    
    ofFloatPixels pixels;
    pixels.allocate(width, height, OF_IMAGE_COLOR);
    
    for (int i = 0; i < width; i++) {
      for (int j = 0; j < height; j++) {
        double* c = som.getMapAt(i, j);
        ofFloatColor col(c[0], c[1], c[2]);
        pixels.setColor(i, j, col);
      }
    }
    
    newPalettePixels.send(std::move(pixels));
  }
}

// Sample 8 colors from the SOM pixels round the edges within a margin
// X..X..X
// .......
// X.....X
// .......
// X..X..X
void SomPalette::updatePalette() {
  // fetch nine points within a margin of the edges
  palette[0] = pixels.getColor(pixels.getWidth()/8, pixels.getHeight()/8);
  palette[1] = pixels.getColor(pixels.getWidth()/2, pixels.getHeight()/8);
  palette[2] = pixels.getColor(7*pixels.getWidth()/8, pixels.getHeight()/8);
  palette[3] = pixels.getColor(pixels.getWidth()/8, pixels.getHeight()/2);
  palette[4] = pixels.getColor(7*pixels.getWidth()/8, pixels.getHeight()/2);
  palette[5] = pixels.getColor(pixels.getWidth()/8, 7*pixels.getHeight()/8);
  palette[6] = pixels.getColor(pixels.getWidth()/2, 7*pixels.getHeight()/8);
  palette[7] = pixels.getColor(7*pixels.getWidth()/8, 7*pixels.getHeight()/8);
  std::sort(palette.begin(), palette.end(),
            [](ofColor a, ofColor b){
              return a.getLightness() < b.getLightness();
            });
}

void SomPalette::update() {
  isNewPalettePixelsReady = false;
  while (newPalettePixels.tryReceive(pixels)) {
    isNewPalettePixelsReady = true;
  }
  if (isNewPalettePixelsReady) {
    if (!paletteTexture.isAllocated()) {
      paletteTexture.allocate(pixels, false);
      paletteTexture.setTextureMinMagFilter(GL_LINEAR, GL_LINEAR); // for interpolation when sampling
      paletteTexture.setTextureWrap(GL_MIRRORED_REPEAT, GL_MIRRORED_REPEAT); // for wrapping when sampling
    }
    paletteTexture.loadData(pixels);
    updatePalette();
  }
}

bool SomPalette::keyPressed(int key) {
  std::string timestamp = ofGetTimestampString();
  if (key == 'S' && paletteTexture.isAllocated()) {
    ofSaveImage(pixels, ofFilePath::getUserHomeDir()+"/Documents/som/"+timestamp+"-snapshot.png", OF_IMAGE_QUALITY_BEST);
    ofFbo fbo;
    fbo.allocate(8 * 64, 64, GL_RGB);
    fbo.begin();
    ofFill();
    for (int i = 0; i < palette.size(); i++) {
      ofSetColor(getColor(i));
      ofDrawRectangle(i*64, 0.0, 64, 64);
    }
    fbo.end();
    ofPixels p;
    fbo.readToPixels(p);
    ofSaveImage(p, ofFilePath::getUserHomeDir()+"/Documents/som/"+timestamp+"-palette.png", OF_IMAGE_QUALITY_BEST);
    return true;
  }
  if (key == 'C') {
    setVisible(!isVisible());
    return true;
  }
  return false;
}

void SomPalette::draw(bool forceVisible, bool paletteOnly) {
  if (!forceVisible && !isVisible()) return;

  ofPushStyle();
  ofEnableBlendMode(OF_BLENDMODE_DISABLED);
  ofSetColor(255);
  
  // full SOM texture
  if (!paletteOnly) {
    if (paletteTexture.isAllocated()) {
      paletteTexture.draw(0, 0, 1.0, 1.0);
    }
  }
  // Discrete palette chips
  float chipWidth = 1.0 / palette.size();
  ofFill();
  for (int i = 0; i < palette.size(); i++) {
    ofSetColor(getColor(i));
    ofDrawRectangle(i*chipWidth, 0.0, chipWidth, chipWidth / 2.0);
  }
  ofPopStyle();
}

ofColor SomPalette::getColorAt(int x, int y) const {
  if (!paletteTexture.isAllocated()) return ofColor::black;
  return pixels.getColor(x, y);
}
