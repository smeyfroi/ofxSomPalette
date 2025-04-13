#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(ofToDataPath("trombone.wav"));
//    audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(ofToDataPath("trombone.wav"));
//    audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(ofToDataPath("Nightsong.wav"));
//    audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(ofToDataPath("Treganna.wav"));
//    audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(ofToDataPath("bells-descending-peal.wav"));
//    audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(ofToDataPath("violin-tune.wav"));
  //  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>();
  audioDataProcessorPtr = std::make_shared<ofxAudioData::Processor>(audioAnalysisClientPtr);
}

//--------------------------------------------------------------
void ofApp::update(){
  audioDataProcessorPtr->update();
  float u = audioDataProcessorPtr->getNormalisedScalarValue(ofxAudioAnalysisClient::AnalysisScalar::complexSpectralDifference, 0.0, 100.0);
  float v = audioDataProcessorPtr->getNormalisedScalarValue(ofxAudioAnalysisClient::AnalysisScalar::spectralCrest, 0.0, 100.0);
  float w = audioDataProcessorPtr->getNormalisedScalarValue(ofxAudioAnalysisClient::AnalysisScalar::zeroCrossingRate, 0.0, 20.0);
 
  std::array<double, 3> instance {
    static_cast<double>(u),
    static_cast<double>(v),
    static_cast<double>(w)
  };
  somPalette.addInstanceData(instance);
  somPalette.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
  somPalette.draw();
}

//--------------------------------------------------------------
void ofApp::exit(){
  audioAnalysisClientPtr->closeStream();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  if (audioAnalysisClientPtr->keyPressed(key)) return;
  if (somPalette.keyPressed(key)) return;
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
