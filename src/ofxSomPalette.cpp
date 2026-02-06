#include "ofxSomPalette.h"
#include "ofTexture.h"

#include <algorithm>
#include <limits>
#include <vector>

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
  shouldWarmStartOnNextInstance = true;
}

void SomPalette::warmStartFromFirstInstance(float mix) {
  warmStartMix.store(mix);
  shouldWarmStartOnNextInstance = true;
}

void SomPalette::addInstanceData(SomInstanceDataT instanceData) {
  if (isIterating()) newInstanceData.send(instanceData);
}

void SomPalette::setColorizerGains(float grayGain, float chromaGain) {
  colorizerGrayGain.store(grayGain);
  colorizerChromaGain.store(chromaGain);
}

// TODO: Make sure we can't be overwhelmed if producer fills queue faster than we consume (e.g. could just do the SOM not the pixels)
void SomPalette::threadedFunction() {
  SomInstanceDataT instanceData;
  
  while (newInstanceData.receive(instanceData)) {
    if (shouldWarmStartOnNextInstance) {
      const float mix = warmStartMix.load();
      const float invMix = 1.0f - mix;

      // Preserve per-cell variation so the palette doesn't collapse to a single color.
      // We bias the map toward the first observed instance, but keep a small, deterministic jitter.
      const float noiseAmp = 0.08f * invMix;

      for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
          double* c = som.getMapAt(i, j);

          // Simple coordinate hash -> [0..1)
          uint32_t h = static_cast<uint32_t>(i * 73856093) ^ static_cast<uint32_t>(j * 19349663);

          for (int z = 0; z < 3; z++) {
            h ^= static_cast<uint32_t>((z + 1) * 83492791);
            h *= 1664525u;
            h += 1013904223u;

            const float n01 = static_cast<float>(h) / static_cast<float>(std::numeric_limits<uint32_t>::max());
            const float n = (n01 * 2.0f - 1.0f) * noiseAmp;

            const float target = ofClamp(static_cast<float>(instanceData[z]) + n, 0.0f, 1.0f);
            c[z] = ofClamp(static_cast<float>(invMix * c[z] + mix * target), 0.0f, 1.0f);
          }
        }
      }
      shouldWarmStartOnNextInstance = false;
    }

    som.updateMap(instanceData.data());
    
    ofFloatPixels pixels;
    pixels.allocate(width, height, OF_IMAGE_COLOR);
    
    const float grayGain = colorizerGrayGain.load();
    const float chromaGain = colorizerChromaGain.load();

    for (int i = 0; i < width; i++) {
      for (int j = 0; j < height; j++) {
        double* c = som.getMapAt(i, j);

        // Feature-space -> RGB colorization.
        // Features are expected in [0..1]:
        //   f0 = centroid, f1 = crest, f2 = zcr
        // Centroid contributes equally to RGB (brightness), while crest/zcr contribute to chroma.
        const float x0 = static_cast<float>(c[0]) - 0.5f;
        const float x1 = static_cast<float>(c[1]) - 0.5f;
        const float x2 = static_cast<float>(c[2]) - 0.5f;

        const float gray = grayGain * x0;

        // Chroma plane from (crest, zcr). Instead of mapping chroma mostly into R/G and leaving B
        // to follow brightness, spread chroma across RGB so "blue" can actually occur.
        const float u = chromaGain * x1;
        // Invert zcr axis so higher zcr can contribute "blue".
        const float v = chromaGain * -x2;

        // 120-degree rotation basis (u,v) -> (r,g,b) with zero-sum chroma.
        constexpr float SQRT3_OVER_2 = 0.8660254037844386f;
        const float r = ofClamp(0.5f + gray + u, 0.0f, 1.0f);
        const float g = ofClamp(0.5f + gray - 0.5f * u + SQRT3_OVER_2 * v, 0.0f, 1.0f);
        const float b = ofClamp(0.5f + gray - 0.5f * u - SQRT3_OVER_2 * v, 0.0f, 1.0f);

        ofFloatColor col(r, g, b);
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
  // Pick the most-separated colors from the SOM field, then sort by lightness.
  // This gives a more varied palette than fixed edge sampling.
  const int w = pixels.getWidth();
  const int h = pixels.getHeight();
  if (w <= 0 || h <= 0) return;

  auto lightness = [](const ofFloatColor& c) {
    // Approximate lightness in [0..1]
    return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
  };

  auto dist2 = [](const ofFloatColor& a, const ofFloatColor& b) {
    float dr = a.r - b.r;
    float dg = a.g - b.g;
    float db = a.b - b.b;
    return dr * dr + dg * dg + db * db;
  };

  std::vector<ofFloatColor> candidates;
  candidates.reserve(static_cast<size_t>(w) * static_cast<size_t>(h));

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      candidates.push_back(pixels.getColor(x, y));
    }
  }

  // Seed with darkest + lightest.
  size_t darkestIndex = 0;
  size_t lightestIndex = 0;
  float minL = std::numeric_limits<float>::infinity();
  float maxL = -std::numeric_limits<float>::infinity();

  for (size_t i = 0; i < candidates.size(); ++i) {
    float l = lightness(candidates[i]);
    if (l < minL) {
      minL = l;
      darkestIndex = i;
    }
    if (l > maxL) {
      maxL = l;
      lightestIndex = i;
    }
  }

  std::vector<ofFloatColor> selected;
  selected.reserve(size);

  std::vector<uint8_t> used(candidates.size(), 0);
  selected.push_back(candidates[darkestIndex]);
  used[darkestIndex] = 1;
  if (lightestIndex != darkestIndex) {
    selected.push_back(candidates[lightestIndex]);
    used[lightestIndex] = 1;
  }

  while (selected.size() < size) {
    size_t bestIndex = 0;
    float bestScore = -1.0f;

    for (size_t i = 0; i < candidates.size(); ++i) {
      if (used[i]) continue;

      float minD2 = std::numeric_limits<float>::infinity();
      for (const auto& s : selected) {
        minD2 = std::min(minD2, dist2(candidates[i], s));
      }

      if (minD2 > bestScore) {
        bestScore = minD2;
        bestIndex = i;
      }
    }

    selected.push_back(candidates[bestIndex]);
    used[bestIndex] = 1;
  }

  for (size_t i = 0; i < size; ++i) {
    palette[i] = selected[i];
  }

  std::sort(palette.begin(), palette.end(), [](ofColor a, ofColor b) { return a.getLightness() < b.getLightness(); });
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
  if (key == 'U' && paletteTexture.isAllocated()) {
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
