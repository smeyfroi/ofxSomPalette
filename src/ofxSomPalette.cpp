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

  // Avoid bright startup flashes before any audio arrives.
  palette.fill(ofColor::black);
  pixels.allocate(width, height, OF_IMAGE_COLOR);
  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      pixels.setColor(x, y, ofFloatColor(0.0f, 0.0f, 0.0f));
    }
  }

  setupSom(initialLearningRate_, numIterations_);
  startThread();
}

SomPalette::~SomPalette() {
  newInstanceData.close();
  newPalettePixels.close();
  waitForThread(true);
}

void SomPalette::setupSom(float initialLearningRate, int numIterations) {
  double minInstance[4] = { 0, 0, 0, 0 };
  double maxInstance[4] = { 1.0, 1.0, 1.0, 1.0 };
  som.setFeaturesRange(4, minInstance, maxInstance);
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

  palette.fill(ofColor::black);
  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      pixels.setColor(x, y, ofFloatColor(0.0f, 0.0f, 0.0f));
    }
  }
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

void SomPalette::setColorizerMaxBrightness(float maxBrightness) {
  colorizerMaxBrightness.store(maxBrightness);
}

void SomPalette::setChipSaturationBias(float bias) {
  chipSaturationBias.store(bias);
}

void SomPalette::setTextureSmoothingSecs(float secs) {
  textureSmoothingSecs.store(secs);
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

          for (int z = 0; z < 4; z++) {
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
    const float maxBrightness = colorizerMaxBrightness.load();

    // Scale the colorizer into the [0, maxBrightness] cube directly. Without this
    // rescaling, the math sits in the [0,1] cube and the channel clamp at maxBrightness
    // pulls high-rms+chromatic cells back toward grey-at-cap (every channel near maxBrightness).
    const float baseline = maxBrightness * 0.5f;
    const float lightnessScale = grayGain * maxBrightness;
    const float chromaScale = chromaGain * maxBrightness;

    for (int i = 0; i < width; i++) {
      for (int j = 0; j < height; j++) {
        double* c = som.getMapAt(i, j);

        // Feature-space -> RGB colorization (Scheme A: explicit LCh allocation, scaled to fit
        // the [0, maxBrightness] cube so the cap doesn't crush chroma differentiation).
        // Features are expected in [0..1]:
        //   f0 = centroid  -> hue chroma u axis
        //   f1 = crest     -> chroma magnitude (saturation gain)
        //   f2 = flatness  -> hue chroma v axis (inverted so high = "blue")
        //   f3 = rms       -> lightness
        const float f0 = static_cast<float>(c[0]);
        const float f1 = static_cast<float>(c[1]);
        const float f2 = static_cast<float>(c[2]);
        const float f3 = static_cast<float>(c[3]);

        // Lightness contribution, scaled to live in [-baseline, +baseline] so baseline+gray
        // sweeps the full [0, maxBrightness] vertical range as f3 sweeps [0,1].
        const float gray = lightnessScale * (f3 - 0.5f);

        const float uDir = (f0 - 0.5f);
        const float vDir = -(f2 - 0.5f); // invert: high flatness -> negative v -> blue
        const float satScale = chromaScale * f1;
        const float u = satScale * uDir;
        const float v = satScale * vDir;

        // 120-degree rotation basis (u,v) -> (r,g,b) with zero-sum chroma.
        // Some clipping still occurs at the joint extremes of lightness + chroma but is much
        // gentler than before; most cells stay inside the cube.
        constexpr float SQRT3_OVER_2 = 0.8660254037844386f;
        const float r = ofClamp(baseline + gray + u, 0.0f, maxBrightness);
        const float g = ofClamp(baseline + gray - 0.5f * u + SQRT3_OVER_2 * v, 0.0f, maxBrightness);
        const float b = ofClamp(baseline + gray - 0.5f * u - SQRT3_OVER_2 * v, 0.0f, maxBrightness);

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

  auto chroma = [](const ofFloatColor& c) {
    const float maxv = std::max({c.r, c.g, c.b});
    const float minv = std::min({c.r, c.g, c.b});
    return maxv - minv;
  };

  const float satBias = chipSaturationBias.load();

  std::vector<ofFloatColor> candidates;
  candidates.reserve(static_cast<size_t>(w) * static_cast<size_t>(h));

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      candidates.push_back(pixels.getColor(x, y));
    }
  }

  // Seed with darkest + lightest. To avoid the "lightest is always grey-at-cap" edge case
  // (any chromatic cell has at least one channel below the cap, so its luminance is lower than
  // a fully-capped grey), search within a luminance band near each extreme and pick the most
  // chromatic cell inside the band. With satBias=0 the band collapses to 0 and we get the
  // original pure-extreme behaviour.
  const float seedBand = std::min(0.3f, satBias * 0.3f);

  float minL = std::numeric_limits<float>::infinity();
  float maxL = -std::numeric_limits<float>::infinity();
  for (const auto& c : candidates) {
    const float l = lightness(c);
    minL = std::min(minL, l);
    maxL = std::max(maxL, l);
  }

  const float lightestThreshold = maxL - seedBand;
  const float darkestThreshold = minL + seedBand;

  size_t darkestIndex = 0;
  size_t lightestIndex = 0;
  float bestLightChroma = -std::numeric_limits<float>::infinity();
  float bestLightL = -std::numeric_limits<float>::infinity();
  float bestDarkChroma = -std::numeric_limits<float>::infinity();
  float bestDarkL = std::numeric_limits<float>::infinity();

  for (size_t i = 0; i < candidates.size(); ++i) {
    const float l = lightness(candidates[i]);
    const float chr = chroma(candidates[i]);

    // Lightest: within the upper band, prefer higher chroma; tie-break by higher luminance.
    if (l >= lightestThreshold) {
      if (chr > bestLightChroma || (chr == bestLightChroma && l > bestLightL)) {
        bestLightChroma = chr;
        bestLightL = l;
        lightestIndex = i;
      }
    }

    // Darkest: within the lower band, prefer higher chroma; tie-break by lower luminance.
    if (l <= darkestThreshold) {
      if (chr > bestDarkChroma || (chr == bestDarkChroma && l < bestDarkL)) {
        bestDarkChroma = chr;
        bestDarkL = l;
        darkestIndex = i;
      }
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
    float bestScore = -std::numeric_limits<float>::infinity();

    for (size_t i = 0; i < candidates.size(); ++i) {
      if (used[i]) continue;

      float minD2 = std::numeric_limits<float>::infinity();
      for (const auto& s : selected) {
        minD2 = std::min(minD2, dist2(candidates[i], s));
      }

      // Reward chromatic candidates so the picker doesn't collapse to neutral
      // greys along the lightness axis (RGB Euclidean distance is biased toward
      // grey-along-L for "spread").
      const float score = std::sqrt(minD2) + satBias * chroma(candidates[i]);

      if (score > bestScore) {
        bestScore = score;
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
  ofFloatPixels incoming;
  bool gotIncoming = false;
  while (newPalettePixels.tryReceive(incoming)) {
    gotIncoming = true;
  }

  isNewPalettePixelsReady = false;
  if (gotIncoming) {
    const float smoothSecs = textureSmoothingSecs.load();
    const bool sameLayout =
      pixels.getWidth() == incoming.getWidth() &&
      pixels.getHeight() == incoming.getHeight() &&
      pixels.getNumChannels() == incoming.getNumChannels() &&
      pixels.getData() != nullptr;

    if (smoothSecs > 1e-6f && sameLayout) {
      // Approximate main-thread frame dt at 30fps; texture smoothing time constant is in seconds.
      const float dt = 1.0f / 30.0f;
      const float alpha = 1.0f - std::exp(-dt / smoothSecs);
      float* dst = pixels.getData();
      const float* src = incoming.getData();
      const size_t n =
        static_cast<size_t>(incoming.getWidth()) *
        static_cast<size_t>(incoming.getHeight()) *
        static_cast<size_t>(incoming.getNumChannels());
      for (size_t i = 0; i < n; ++i) {
        dst[i] = dst[i] + alpha * (src[i] - dst[i]);
      }
    } else {
      pixels = incoming;
    }
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
