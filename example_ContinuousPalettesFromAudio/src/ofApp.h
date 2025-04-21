#pragma once

#include "ofMain.h"
#include "ofxContinuousSomPalette.hpp"
#include "ofxAudioAnalysisClient.h"
#include "ofxAudioData.h"

class ofApp: public ofBaseApp{
public:
  void setup();
  void update();
  void draw();
  void exit();
  
  void keyPressed(int key);
  void keyReleased(int key);
  void mouseMoved(int x, int y);
  void mouseDragged(int x, int y, int button);
  void mousePressed(int x, int y, int button);
  void mouseReleased(int x, int y, int button);
  void windowResized(int w, int h);
  void dragEvent(ofDragInfo dragInfo);
  void gotMessage(ofMessage msg);
  
  std::shared_ptr<ofxAudioAnalysisClient::LocalGistClient> audioAnalysisClientPtr;
  std::shared_ptr<ofxAudioData::Processor> audioDataProcessorPtr;
  
  ContinuousSomPalette somPalette { 16, 16, 0.1, 500 };
};
