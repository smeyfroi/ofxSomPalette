#include "ofxContinuousSomPalette.hpp"

ContinuousSomPalette::ContinuousSomPalette(int width_, int height_, float initialLearningRate_, int numIterations_) :
width { width_ },
height { height_ },
initialLearningRate { initialLearningRate_ },
numIterations { numIterations_ },
activeSomPalette { 0 },
nextActiveSomPalette { -1 }
{
  std::for_each(somPalettePtrs.begin(),
                somPalettePtrs.end(),
                [this](auto& p){ p = std::make_unique<SomPalette>(width, height, initialLearningRate, numIterations); });
}

void ContinuousSomPalette::addInstanceData(SomInstanceDataT instanceData) {
  somPalettePtrs[activeSomPalette]->addInstanceData(instanceData);
  if (nextActiveSomPalette != -1) somPalettePtrs[nextActiveSomPalette]->addInstanceData(instanceData);
}

void ContinuousSomPalette::switchPalette() {
  if (nextActiveSomPalette == -1) return;
  activeSomPalette = nextActiveSomPalette;
  nextActiveSomPalette = -1;
}

void ContinuousSomPalette::update() {
  auto& active = somPalettePtrs[activeSomPalette];
  if (active->getCurrentIteration() >= active->getNumIterations()) {
    switchPalette();
  } else if (nextActiveSomPalette == -1 && active->getCurrentIteration() > active->getNumIterations() * startNextPaletteAt) {
    nextActiveSomPalette = (activeSomPalette + 1) % somPalettePtrs.size();
    somPalettePtrs[nextActiveSomPalette] = std::make_unique<SomPalette>(width, height, initialLearningRate, numIterations);
  }
  somPalettePtrs[activeSomPalette]->update();
  if (nextActiveSomPalette != -1) somPalettePtrs[nextActiveSomPalette]->update();
}

// Return true if the next palette is ready to be switched to (20% of iterations done)
bool ContinuousSomPalette::nextPaletteIsReady() const {
  if (nextActiveSomPalette == -1) return false;
  auto& nextActive = somPalettePtrs[nextActiveSomPalette];
  return (nextActive->getCurrentIteration() > nextActive->getNumIterations() * paletteReadyAt);
}

bool ContinuousSomPalette::keyPressed(int key) {
  if (key == 'C') {
    setVisible(!isVisible());
    return true;
  }
  return somPalettePtrs[activeSomPalette]->keyPressed(key);
}

void ContinuousSomPalette::draw() {
  somPalettePtrs[activeSomPalette]->draw(visible, false);
  ofPushMatrix();
  ofTranslate(0.0, 1.0 / somPalettePtrs[activeSomPalette]->size * 0.5);
  if (nextActiveSomPalette != -1) somPalettePtrs[nextActiveSomPalette]->draw(visible, true);
  ofPopMatrix();
}

ofColor ContinuousSomPalette::getColor(int i) const {
  return somPalettePtrs[activeSomPalette]->getColor(i);
}

bool ContinuousSomPalette::isVisible() const {
  return visible;
}

void ContinuousSomPalette::setVisible(bool visible_) {
  visible = visible_;
}

ofFloatPixels ContinuousSomPalette::getPixels() const {
  return somPalettePtrs[activeSomPalette]->getPixels();
}
