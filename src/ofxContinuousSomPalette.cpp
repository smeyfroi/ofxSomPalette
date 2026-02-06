#include "ofxContinuousSomPalette.hpp"

#include <algorithm>

ContinuousSomPalette::ContinuousSomPalette(int width_, int height_, float initialLearningRate_, int numIterations_)
: width { width_ }
, height { height_ }
, initialLearningRate { initialLearningRate_ }
, numIterations { numIterations_ }
{
  hopFrames = std::max(1, windowFrames / 2);

  std::for_each(somPalettePtrs.begin(), somPalettePtrs.end(), [this](auto& p) {
    p = std::make_unique<SomPalette>(width, height, initialLearningRate, numIterations);
    p->setColorizerGains(colorizerGrayGain, colorizerChromaGain);
  });

  blendedPixels.allocate(width, height, OF_IMAGE_COLOR);
}

void ContinuousSomPalette::addInstanceData(SomInstanceDataT instanceData) {
  somPalettePtrs[blendFromIndex]->addInstanceData(instanceData);
  somPalettePtrs[blendToIndex]->addInstanceData(instanceData);
}

void ContinuousSomPalette::switchPalette() {
  performHop();
}

bool ContinuousSomPalette::nextPaletteIsReady() const {
  return (somPalettePtrs[blendToIndex]->getPixelsRef().getWidth() > 0);
}

void ContinuousSomPalette::update() {
  ++frameCount;

  while (frameCount - lastHopFrameCount >= hopFrames) {
    performHop();
  }

  somPalettePtrs[blendFromIndex]->update();
  somPalettePtrs[blendToIndex]->update();

  updateBlendedOutputs();
}

bool ContinuousSomPalette::keyPressed(int key) {
  if (key == 'C') {
    setVisible(!isVisible());
    return true;
  }
  return somPalettePtrs[blendFromIndex]->keyPressed(key);
}

void ContinuousSomPalette::draw() {
  somPalettePtrs[blendFromIndex]->draw(visible, false);
  ofPushMatrix();
  ofTranslate(0.0, 1.0 / somPalettePtrs[blendFromIndex]->size * 0.5);
  somPalettePtrs[blendToIndex]->draw(visible, true);
  ofPopMatrix();
}

ofColor ContinuousSomPalette::getColor(int i) const {
  const float alpha = getBlendAlpha();
  return somPalettePtrs[blendFromIndex]->getColor(i).getLerped(somPalettePtrs[blendToIndex]->getColor(i), alpha);
}

bool ContinuousSomPalette::isVisible() const {
  return visible;
}

void ContinuousSomPalette::setVisible(bool visible_) {
  visible = visible_;
}

const ofFloatPixels& ContinuousSomPalette::getPixelsRef() const {
  return blendedPixels;
}

const ofTexture* ContinuousSomPalette::getActiveTexturePtr() const {
  if (blendedTexture.isAllocated()) return &blendedTexture;
  return &somPalettePtrs[blendFromIndex]->getTexture();
}

const ofTexture* ContinuousSomPalette::getNextTexturePtr() const {
  return &somPalettePtrs[blendToIndex]->getTexture();
}

void ContinuousSomPalette::setNumIterations(int numIterations_) {
  numIterations = numIterations_;
  for (auto& sp : somPalettePtrs) {
    sp->setNumIterations(numIterations_);
  }
}

void ContinuousSomPalette::setWindowFrames(int windowFrames_) {
  windowFrames = std::max(2, windowFrames_);
  hopFrames = std::max(1, windowFrames / 2);

  // Restart timing so window length change doesn't cause an immediate hop.
  frameCount = 0;
  lastHopFrameCount = 0;
}

void ContinuousSomPalette::setColorizerGains(float grayGain, float chromaGain) {
  colorizerGrayGain = grayGain;
  colorizerChromaGain = chromaGain;
  for (auto& sp : somPalettePtrs) {
    sp->setColorizerGains(colorizerGrayGain, colorizerChromaGain);
  }
}

void ContinuousSomPalette::performHop() {
  lastHopFrameCount = frameCount;

  blendFromIndex = blendToIndex;
  blendToIndex = (blendFromIndex + 1) % somPalettePtrs.size();

  somPalettePtrs[blendToIndex] = std::make_unique<SomPalette>(width, height, initialLearningRate, numIterations);
  somPalettePtrs[blendToIndex]->setColorizerGains(colorizerGrayGain, colorizerChromaGain);

  blendedTexture.clear();
}

float ContinuousSomPalette::getBlendAlpha() const {
  if (hopFrames <= 0) return 1.0f;

  float t = static_cast<float>(frameCount - lastHopFrameCount) / static_cast<float>(hopFrames);
  t = ofClamp(t, 0.0f, 1.0f);

  // Smoothstep for gentle crossfade.
  return t * t * (3.0f - 2.0f * t);
}

void ContinuousSomPalette::updateBlendedOutputs() {
  const auto& a = somPalettePtrs[blendFromIndex]->getPixelsRef();
  const auto& b = somPalettePtrs[blendToIndex]->getPixelsRef();

  const int w = a.getWidth();
  const int h = a.getHeight();
  if (w <= 0 || h <= 0) return;

  if (!blendedPixels.isAllocated() || blendedPixels.getWidth() != w || blendedPixels.getHeight() != h) {
    blendedPixels.allocate(w, h, OF_IMAGE_COLOR);
  }

  const float alpha = getBlendAlpha();

  const float* srcA = a.getData();
  const float* srcB = (b.getWidth() == w && b.getHeight() == h) ? b.getData() : nullptr;
  float* dst = blendedPixels.getData();

  const size_t n = static_cast<size_t>(w) * static_cast<size_t>(h) * 3;
  if (srcB) {
    for (size_t i = 0; i < n; ++i) {
      dst[i] = ofLerp(srcA[i], srcB[i], alpha);
    }
  } else {
    std::copy(srcA, srcA + n, dst);
  }

  if (!blendedTexture.isAllocated()) {
    blendedTexture.allocate(blendedPixels, false);
    blendedTexture.setTextureMinMagFilter(GL_LINEAR, GL_LINEAR);
    blendedTexture.setTextureWrap(GL_MIRRORED_REPEAT, GL_MIRRORED_REPEAT);
  }
  blendedTexture.loadData(blendedPixels);
}
