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
                [=](auto& p){ p = std::make_unique<SomPalette>(width, height, initialLearningRate, numIterations); });
}

void ContinuousSomPalette::addInstanceData(SomInstanceDataT instanceData) {
  somPalettePtrs[activeSomPalette]->addInstanceData(instanceData);
  if (nextActiveSomPalette != -1) somPalettePtrs[nextActiveSomPalette]->addInstanceData(instanceData);
}

void ContinuousSomPalette::update() {
  auto& active = somPalettePtrs[activeSomPalette];
  // Swap active to next palette at end of iterations, reset nextActive
  if (active->getCurrentIteration() >= active->getNumIterations()) {
    activeSomPalette = nextActiveSomPalette;
    nextActiveSomPalette = -1;
    ofLogNotice() << "Switched to next palette " << activeSomPalette;
  }
  // Else create next palette at 80% of iterations
  else if (nextActiveSomPalette == -1 && active->getCurrentIteration() > active->getNumIterations() * 0.6) {
    nextActiveSomPalette = (activeSomPalette + 1) % somPalettePtrs.size();
    somPalettePtrs[nextActiveSomPalette] = std::make_unique<SomPalette>(width, height, initialLearningRate, numIterations);
    ofLogNotice() << "Created next palette " << nextActiveSomPalette;
  }
  somPalettePtrs[activeSomPalette]->update();
  if (nextActiveSomPalette != -1) somPalettePtrs[nextActiveSomPalette]->update();
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
  ofTranslate(0.0, 140.0);
  ofScale(1.0, 0.3);
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
