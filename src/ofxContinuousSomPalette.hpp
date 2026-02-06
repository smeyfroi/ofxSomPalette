#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include "ofMain.h"
#include "ofxSomPalette.h"

// Maintains overlapping palettes to approximate a sliding time window.
//
// Two palettes are trained in parallel on the same recent data, and the output is a smooth crossfade.
// The crossfade/hop cadence is frame-based (not dependent on how many training samples are queued).
class ContinuousSomPalette {
public:
  ContinuousSomPalette(int width_ = 16, int height_ = 16, float initialLearningRate_ = 0.015f, int numIterations_ = 4000);
  void addInstanceData(SomInstanceDataT instanceData);
  void update(); // move pixels into a GL texture on main thread

  // Forces an immediate hop (reset + restart crossfade).
  void switchPalette();

  // In sliding-window mode the "next" palette is always available.
  bool nextPaletteIsReady() const;

  bool keyPressed(int key);
  void draw();
  ofColor getColor(int i) const;
  bool isVisible() const;
  void setVisible(bool visible_);

  // Blended pixels/texture represent the current sliding-window palette.
  const ofFloatPixels& getPixelsRef() const;
  const ofTexture* getActiveTexturePtr() const;
  const ofTexture* getNextTexturePtr() const;

  // Training iteration count for each underlying palette.
  void setNumIterations(int numIterations); // propagate to underlying somPalettes

  // Window cadence in frames (e.g. 15s * 30fps = 450). The hop/crossfade is half this.
  void setWindowFrames(int windowFrames);

  void setColorizerGains(float grayGain, float chromaGain);

  int width, height;
  float initialLearningRate;
  int numIterations;

private:
  bool visible = false;

  std::array<std::unique_ptr<SomPalette>, 2> somPalettePtrs;

  // Sliding-window state
  int blendFromIndex { 0 };
  int blendToIndex { 1 };

  int windowFrames { 450 };
  int hopFrames { 225 };
  int64_t frameCount { 0 };
  int64_t lastHopFrameCount { 0 };

  // Blended outputs
  ofFloatPixels blendedPixels;
  ofTexture blendedTexture;

  float colorizerGrayGain { 1.0f };
  float colorizerChromaGain { 1.25f };

  void performHop();
  float getBlendAlpha() const;
  void updateBlendedOutputs();
};
