/*
Example combining FaceTracker2 Regression and Classification
*/

#include "ofApp.h"


//--------------------------------------------------------------
void ofApp::setup(){
    
    ofSetFrameRate(60);
    ofSetVerticalSync(true);
    
    largeFont.load("verdana.ttf", 12, true, true);
    largeFont.setLineHeight(14.0f);
    smallFont.load("verdana.ttf", 9, true, true);
    smallFont.setLineHeight(10.0f);
    hugeFont.load("verdana.ttf", 36, true, true);
    hugeFont.setLineHeight(38.0f);
    
    //Initialize the training and info variables
    infoText_R = "";
    trainingModeActive_R = false;
    recordTrainingData_R = false;
    predictionModeActive_R = false;
    drawInfo = true;
    
    //Select the inputs: Gestures, orientations or raw inputs
    rawBool = false;
    gestureBool = true;
    orientationBool = false;
    
    trainingInputs = GESTUREINPUTS*gestureBool+ORIENTATIONINPUTS*orientationBool+RAWINPUTS*rawBool; //RAWINPUTS=136, ORIENTATIONINPUTS=9, GESTUREINPUTS=5
    trainingData_R.setInputAndTargetDimensions( trainingInputs, 2 );
    
    //set the default classifier
    setRegressifier( LINEAR_REGRESSION );
    
    //CLASSIFICATION
    //Initialize the training and info variables
    infoText_C = "";
    trainingClassLabel_C = 1;
    record_C = false;
    //drawInfo_C = true;
    
    trainingData_C.setNumDimensions( trainingInputs );
    
    //set the default classifier
    ANBC naiveBayes;
    naiveBayes.enableNullRejection( false );
    naiveBayes.setNullRejectionCoeff( 3 );
    pipeline_C.setClassifier( naiveBayes );
    
    
    //Facetracker
    //Select the inputs: Gestures, orientations or raw inputs
    rawBool = false;
    gestureBool = true;
    orientationBool = false;
    
    //What to draw
    drawNumbers = false;
    drawFace = true;
    drawPose = false;
    drawVideo = true;
    
    
    // Setup grabber
    grabber.setup(1280,720);
    //grabber.setup(960,540);
    
    // Setup tracker
    tracker.setup("../../../model/shape_predictor_68_face_landmarks.dat");
    
    
    //GUI
    gui.setup();
    gui.setPosition(ofGetWidth()-200, 110);
//    gui.setSize(300, 75);
//    gui.setDefaultWidth(300);
//    gui.setDefaultHeight(75);
    gui.add(val1.setup("output value 1", 0.7, 0.0, 1.0));
    gui.add(val2.setup("output value 2", 0.7, 0.0, 1.0));
    gui.add(smoothing.setup("smoothing", 0.85, 0.0, 1.0));
    
}

//--------------------------------------------------------------
void ofApp::update(){
    
    
    //FaceTracker2
    grabber.update();
    
    // Update tracker when there are new frames
    if(grabber.isFrameNew()){
        tracker.update(grabber);
    }
    VectorFloat trainingSample(trainingInputs);
    VectorFloat inputVector(trainingInputs);
    
    if ( tracker.size()>0) {
        
        
        //RAW (136 values)
        if (rawBool) {
            
            //Send all raw points (68 facepoints with x+y = 136 values) (only 1 face)
            auto facePoints = tracker.getInstances()[0].getLandmarks().getImageFeature(ofxFaceTracker2Landmarks::ALL_FEATURES);
            
            for (int i = 0; i<facePoints.size(); i++) {
                ofPoint p = facePoints.getVertices()[i].getNormalized(); //only values from 0-1. Experiment with this, and try to send non-normalized as well
                
                trainingSample[i] = p.x;
                trainingSample[i + facePoints.size()] = p.y; //Not that elegant...
            }
        }
        
        
        //ORIENTATION (9 values)
        if (orientationBool) {
            for (int i = 0; i<=2; i++) {
                for (int j = 0; j<=2; j++) {
                    trainingSample[i+RAWINPUTS*rawBool] = tracker.getInstances()[0].getPoseMatrix().getRowAsVec3f(i)[j];
                }
            }
        }
        
        
        //GESTURES (5 values)
        if (gestureBool) {
            trainingSample[0+RAWINPUTS*rawBool+ORIENTATIONINPUTS*orientationBool] = getGesture(RIGHT_EYE_OPENNESS);
            trainingSample[1+RAWINPUTS*rawBool+ORIENTATIONINPUTS*orientationBool] = getGesture(LEFT_EYE_OPENNESS);
            trainingSample[2+RAWINPUTS*rawBool+ORIENTATIONINPUTS*orientationBool] = getGesture(RIGHT_EYEBROW_HEIGHT);
            trainingSample[3+RAWINPUTS*rawBool+ORIENTATIONINPUTS*orientationBool] = getGesture(LEFT_EYEBROW_HEIGHT);
            trainingSample[4+RAWINPUTS*rawBool+ORIENTATIONINPUTS*orientationBool] = getGesture(MOUTH_HEIGHT);

        }
        
        
        //Update the training mode if needed
        if( trainingModeActive_R){
            
            //Check to see if the countdown timer has elapsed, if so then start the recording
            if( !recordTrainingData_R ){
                if( trainingTimer_R.timerReached() ){
                    recordTrainingData_R = true;
                    trainingTimer_R.start( RECORDING_TIME );
                }
            }else{
                //We should be recording the training data - check to see if we should stop the recording
                if( trainingTimer_R.timerReached() ){
                    trainingModeActive_R = false;
                    recordTrainingData_R = false;
                }
            }
            
            if( recordTrainingData_R ){
 
                VectorFloat targetVector(2);
                targetVector[0] = val1;
                targetVector[1] = val2;

                
                if( !trainingData_R.addSample(trainingSample,targetVector) ){
                    infoText_R = "WARNING: Failed to add training sample to training data!";
                }
            }
        }
        
        inputVector = trainingSample;
        
        //Update the prediction mode if active
        if( predictionModeActive_R ){
            
            if( pipeline_R.predict( inputVector ) ){
                rawVal1 = ofClamp(pipeline_R.getRegressionData()[0],0.0, 1.0);
                rawVal2 = ofClamp(pipeline_R.getRegressionData()[1],0.0, 1.0);

            }else{
                infoText_R = "ERROR: Failed to run prediction!";
            }
        }
    }
    
    if (predictionModeActive_R) {
        val1 = val1 + ( rawVal1 - val1 ) * (1- smoothing);
        val2 = val2 + ( rawVal2 - val2 ) * (1- smoothing);
    }
    
    
    //If we are recording training data, then add the current sample to the training data set
    if( record_C ){
        trainingData_C.addSample( trainingClassLabel_C, trainingSample );
    }
    
    //If the pipeline has been trained, then run the prediction
    if( pipeline_C.getTrained() && predictionModeActive_C ){
        pipeline_C.predict( trainingSample );
        predictionPlot_C.update( pipeline_C.getClassLikelihoods() );
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    ofBackground(225, 225, 225);
    
    //FaceTracker draw
    ofSetColor(255);
    // Draw camera image
    if (drawVideo) grabber.draw(0, 0); //Default
    
    // Draw estimated 3d pose
    if (drawPose) tracker.drawDebugPose();
    
    //Draw all the individual points / numbers for all tracked faces
    for (int i = 0; i<tracker.size(); i ++) {
        
        if(drawFace) {
            ofPolyline jaw = tracker.getInstances()[i].getLandmarks().getImageFeature(ofxFaceTracker2Landmarks::JAW);
            ofPolyline left_eyebrow = tracker.getInstances()[i].getLandmarks().getImageFeature(ofxFaceTracker2Landmarks::LEFT_EYEBROW);
            ofPolyline right_eyebrow = tracker.getInstances()[i].getLandmarks().getImageFeature(ofxFaceTracker2Landmarks::RIGHT_EYEBROW);
            ofPolyline left_eye_top = tracker.getInstances()[i].getLandmarks().getImageFeature(ofxFaceTracker2Landmarks::LEFT_EYE_TOP);
            ofPolyline right_eye_top = tracker.getInstances()[i].getLandmarks().getImageFeature(ofxFaceTracker2Landmarks::RIGHT_EYE_TOP);
            ofPolyline left_eye = tracker.getInstances()[i].getLandmarks().getImageFeature(ofxFaceTracker2Landmarks::LEFT_EYE);
            ofPolyline right_eye = tracker.getInstances()[i].getLandmarks().getImageFeature(ofxFaceTracker2Landmarks::RIGHT_EYE);
            ofPolyline outher_mouth = tracker.getInstances()[i].getLandmarks().getImageFeature(ofxFaceTracker2Landmarks::OUTER_MOUTH);
            ofPolyline inner_mouth = tracker.getInstances()[i].getLandmarks().getImageFeature(ofxFaceTracker2Landmarks::INNER_MOUTH);
            ofPolyline nose_bridge = tracker.getInstances()[i].getLandmarks().getImageFeature(ofxFaceTracker2Landmarks::NOSE_BRIDGE);
            ofPolyline nose_base = tracker.getInstances()[i].getLandmarks().getImageFeature(ofxFaceTracker2Landmarks::NOSE_BASE);
            
            jaw.draw();
            left_eyebrow.draw();
            right_eyebrow.draw();
            left_eye_top.draw();
            right_eye_top.draw();
            left_eye.draw();
            right_eye.draw();
            outher_mouth.draw();
            inner_mouth.draw();
            nose_bridge.draw();
            nose_base.draw();
        }
        
        for (int j = 0; j< tracker.getInstances()[i].getLandmarks().getImagePoints().size(); j++) {
            if (drawNumbers) ofDrawBitmapString(j, tracker.getInstances()[i].getLandmarks().getImagePoint(j));
        }
    }
    

    if( trainingModeActive_R ){
        if( !recordTrainingData_R ){
            ofSetColor(255, 204, 0);
            string txt = "PREP";
            ofRectangle bounds = hugeFont.getStringBoundingBox(txt,0,0);
            hugeFont.drawString(txt,ofGetWidth()-25-bounds.width,ofGetHeight()-25-bounds.height);
        }else{
            ofSetColor(255,0,0);
            string txt = "REC";
            ofRectangle bounds = hugeFont.getStringBoundingBox(txt,0,0);
            hugeFont.drawString(txt,ofGetWidth()-25-bounds.width,ofGetHeight()-25-bounds.height);
        }
    }
    
    
    
    //Draw the info text
    if( drawInfo ){
        float textX = 10;
        float textY = 25;
        float textSpacer = smallFont.getLineHeight() * 1.5;
        
        ofFill();
        ofSetColor(100,100,100);
        ofDrawRectangle( 5, 5, 290, 720 -8 );
        ofSetColor( 255, 255, 255 );

        largeFont.drawString( "EYE CONDUCTOR ML EXAMPLE", textX, textY ); textY += textSpacer;
        
        smallFont.drawString( "Framerate : "+ofToString(ofGetFrameRate()), textX, textY ); textY += textSpacer;
        smallFont.drawString( "Tracker thread framerate : "+ofToString(tracker.getThreadFps()), textX, textY ); textY += textSpacer;
        textY += textSpacer;
        
        largeFont.drawString( "GLOBAL CONTROLS", textX, textY ); textY += textSpacer;
        smallFont.drawString( "[i]: Toogle Info", textX, textY ); textY += textSpacer;
        smallFont.drawString( "[a] Input all raw points: "+ofToString(rawBool), textX, textY ); textY += textSpacer;
        smallFont.drawString( "[g] Input gestures: "+ofToString(gestureBool), textX, textY ); textY += textSpacer;
        smallFont.drawString( "[o] Input orientation: "+ofToString(orientationBool), textX, textY ); textY += textSpacer;
        smallFont.drawString( "Total input values: "+ofToString(trainingInputs), textX, textY ); textY += textSpacer;
        textY += textSpacer;
        
        smallFont.drawString( "[n] draw numbers: "+ofToString(drawNumbers), textX, textY ); textY += textSpacer;
        smallFont.drawString( "[f] draw face: "+ofToString(drawFace), textX, textY ); textY += textSpacer;
        smallFont.drawString( "[p] draw pose: "+ofToString(drawPose), textX, textY ); textY += textSpacer;
        smallFont.drawString( "[v] draw video: "+ofToString(drawVideo), textX, textY ); textY += textSpacer;
        textY += textSpacer;
        
        //REGRESSION
        largeFont.drawString( "REGRESSION CONTROLS", textX, textY ); textY += textSpacer;
        smallFont.drawString( "[r]: Record Sample", textX, textY ); textY += textSpacer;
        smallFont.drawString( "[l]: Load Model", textX, textY ); textY += textSpacer;
        smallFont.drawString( "[s]: Save Model", textX, textY ); textY += textSpacer;
        smallFont.drawString( "[t]: Train Model", textX, textY ); textY += textSpacer;
        smallFont.drawString( "[d]: Pause Model", textX, textY ); textY += textSpacer;
        smallFont.drawString( "[c]: Clear Training Data", textX, textY ); textY += textSpacer;
        smallFont.drawString( "[tab]: Select Regressifier", textX, textY ); textY += textSpacer;
        textY += textSpacer;
        
        smallFont.drawString( "Regressifier: " + regressifierTypeToString( regressifierType ), textX, textY ); textY += textSpacer;
        
        smallFont.drawString( "Recording: " + ofToString( recordTrainingData_R ), textX, textY ); textY += textSpacer;
        smallFont.drawString( "Num Samples: " + ofToString( trainingData_R.getNumSamples() ), textX, textY ); textY += textSpacer;
        smallFont.drawString( "Prediction mode active: " + ofToString( predictionModeActive_R), textX, textY ); textY += textSpacer;
        ofSetColor(255,241,0);
        smallFont.drawString( infoText_R, textX, textY ); textY += textSpacer;
        ofSetColor(255);
        textY += textSpacer;
        
        
        //CLASSIFICATION
        largeFont.drawString( "CLASSIFICATION CONTROLS", textX, textY ); textY += textSpacer;
        smallFont.drawString( "[1-9]: Set Class Label", textX, textY ); textY += textSpacer;
        smallFont.drawString( "[R]: Record Sample", textX, textY ); textY += textSpacer;
        smallFont.drawString( "[L]: Load Model", textX, textY ); textY += textSpacer;
        smallFont.drawString( "[S]: Save Model", textX, textY ); textY += textSpacer;
        smallFont.drawString( "[T]: Train Model", textX, textY ); textY += textSpacer;
        smallFont.drawString( "[D]: Pause Model", textX, textY ); textY += textSpacer;
        smallFont.drawString( "[C]: Clear Training Data", textX, textY ); textY += textSpacer;
        smallFont.drawString( "[enter/return]: Select Classifier", textX, textY ); textY += textSpacer*2;
        
        smallFont.drawString( "Classifier: " + classifierTypeToString(classifierType), textX, textY ); textY += textSpacer;
        smallFont.drawString( "Class Label: " + ofToString( trainingClassLabel_C ), textX, textY ); textY += textSpacer;
        smallFont.drawString( "Recording: " + ofToString( record_C ), textX, textY ); textY += textSpacer;
        smallFont.drawString( "Num Samples: " + ofToString( trainingData_C.getNumSamples() ), textX, textY ); textY += textSpacer;
        smallFont.drawString( "Prediction mode active: " + ofToString( predictionModeActive_C), textX, textY ); textY += textSpacer;
        ofSetColor(255,241,0);
        smallFont.drawString( infoText_C, textX, textY ); textY += textSpacer;
        ofSetColor(255);
        textY += textSpacer;
        
        ofSetColor(0);
        smallFont.drawString( "INSTRUCTIONS REGRESSION: \n \n 1) Set the height and width of the output sliders \n \n 2) Press [r] to record some training samples containing your selected facial features (gestures/orientation/raw points) and output slider values. \n \n 3) Repeat step 1) and 2) with different output slider values and height and different facial expressions / head orientations \n \n 4) Press [t] to train. Move your face and see the changes in the output slider values based on your facial orientation and expression \n \n \n \n 5) INSTRUCTIONS CLASSIFICATION: Like regression but using keys [1-9], [R] (as a toogle) and [T]",  textX, 750 );
        
    }
    
    
    //CLASSIFICATION
    int graphX = 300 ;
    int graphY = 5;
    int graphW = ofGetWidth() - 305;
    int graphH = 100;
    
    if( pipeline_C.getTrained() ){
        predictionPlot_C.draw( graphX, graphY, graphW, graphH ); graphY += graphH * 1.1;
    }
    
    ofSetColor(0,0,0);
    hugeFont.drawString(ofToString(pipeline_C.getPredictedClassLabel()), ofGetWidth()/2, ofGetHeight()/2);

    
    gui.draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
    infoText_R = "";
    infoText_C = "";
    bool buildTexture = false;
    
    switch ( key) {
            
            //GENERAL
        case 'g': //Toogle gesture inputs and update length of trainingInputs
            gestureBool =! gestureBool;
            trainingInputs = GESTUREINPUTS*gestureBool+ORIENTATIONINPUTS*orientationBool+RAWINPUTS*rawBool; //GESTUREINPUTS=5, ORIENTATIONINPUTS=9, RAWINPUTS=136
            trainingData_R.setInputAndTargetDimensions( trainingInputs, 2 );
            break;
            
        case 'o': //Toogle orientation inputs and update length of trainingInputs
            orientationBool =! orientationBool;
            trainingInputs = GESTUREINPUTS*gestureBool+ORIENTATIONINPUTS*orientationBool+RAWINPUTS*rawBool; //GESTUREINPUTS=5, ORIENTATIONINPUTS=9, RAWINPUTS=136
            trainingData_R.setInputAndTargetDimensions( trainingInputs, 2 );
            break;
            
        case 'a': //Toogle raw inputs and update length of trainingInputs
            rawBool =! rawBool;
            trainingInputs = GESTUREINPUTS*gestureBool+ORIENTATIONINPUTS*orientationBool+RAWINPUTS*rawBool; //GESTUREINPUTS=5, ORIENTATIONINPUTS=9, RAWINPUTS=136
            trainingData_R.setInputAndTargetDimensions( trainingInputs, 2 );
            break;
            
        case 'i':
            drawInfo = !drawInfo;
            break;
            
        case 'n':
            drawNumbers = !drawNumbers;
            break;
            
        case 'f':
            drawFace = !drawFace;
            break;
            
        case 'p':
            drawPose = !drawPose;
            break;
            
        case 'v':
            drawVideo = !drawVideo;
            break;

            
            //REGRESSION
        case 'r':
            predictionModeActive_R = false;
            trainingModeActive_R = true;
            recordTrainingData_R = false;
            trainingTimer_R.start( PRE_RECORDING_COUNTDOWN_TIME );
            break;
        case 't':
            if( pipeline_R.train( trainingData_R ) ){
                infoText_R = "Pipeline Trained";
                predictionModeActive_R = true;
            }else infoText_R = "WARNING: Failed to train pipeline";
            break;
        case 's':
            if( trainingData_R.save( ofToDataPath("TrainingData_R.grt") ) ){
                infoText_R = "Training data saved to file";
            }else infoText_R = "WARNING: Failed to save training data to file";
            break;
        case 'l':
            if( trainingData_R.load( ofToDataPath("TrainingData_R.grt") ) ){
                infoText_R = "Training data loaded from file";
            }else infoText_R = "WARNING: Failed to load training data from file";
            break;
        
        case 'd':
            predictionModeActive_R =! predictionModeActive_R;
            infoText_R = "Model paused";
            break;
            
        case 'c':
            trainingData_R.clear();
            infoText_R = "Training data cleared";
            break;
            
            
        case OF_KEY_TAB:
            setRegressifier( ++this->regressifierType % NUM_REGRESSIFIERS );
            break;
            
            
        //CLASSIFICATION
            
        case OF_KEY_RETURN:
            setClassifier( ++this->classifierType % NUM_CLASSIFIERS );
            break;
            
        case '1':
            trainingClassLabel_C = 1;
            break;
        case '2':
            trainingClassLabel_C = 2;
            break;
        case '3':
            trainingClassLabel_C = 3;
            break;
        case '4':
            trainingClassLabel_C = 4;
            break;
        case '5':
            trainingClassLabel_C = 5;
            break;
        case '6':
            trainingClassLabel_C = 6;
            break;
        case '7':
            trainingClassLabel_C = 7;
            break;
        case '8':
            trainingClassLabel_C = 8;
            break;
        case '9':
            trainingClassLabel_C = 9;
            break;
        
        case 'R':
            
            record_C = !record_C;
            
            break;
        
        case 'T':
            if( pipeline_C.train( trainingData_C ) ){
                infoText_C = "Pipeline Trained";
                predictionPlot_C.setup( 500, pipeline_C.getNumClasses(), "prediction likelihoods" );
                predictionPlot_C.setDrawGrid( true );
                predictionPlot_C.setDrawInfoText( true );
                predictionPlot_C.setFont( smallFont );
                
                predictionModeActive_C = true;
            }else infoText_C = "WARNING: Failed to train pipeline";
            break;
        
        case 'L':
            if( trainingData_C.load( ofToDataPath("TrainingData_C.grt") ) ){
                infoText_C = "Training data loaded from file";
            }else infoText_C = "WARNING: Failed to load training data from file";
            break;

        case 'S':
            if( trainingData_C.save( ofToDataPath("TrainingData_C.grt") ) ){
                infoText_C = "Training data saved to file";
            }else infoText_C = "WARNING: Failed to save training data to file";
            break;
        
        case 'C':
            trainingData_C.clear();
            infoText_C = "Training data cleared";
            break;
            
        case 'D':
            predictionModeActive_C =! predictionModeActive_C;
            infoText_C = "Model paused";
        
        
        default:
            break;
    }
    
}

//--------------------------------------------------------------
bool ofApp::setRegressifier( const int type ){
    
    LinearRegression linearRegression;
    LogisticRegression logisticRegression;
    MLP mlp;
    
    this->regressifierType = type;
    
    pipeline_R.clear();
    
    switch( regressifierType ){
        case LINEAR_REGRESSION:
            pipeline_R << MultidimensionalRegression(linearRegression,true);
            break;
        case LOGISTIC_REGRESSION:
            pipeline_R << MultidimensionalRegression(logisticRegression,true);
            break;
        case NEURAL_NET:
        {
            unsigned int numInputNeurons = trainingData_R.getNumInputDimensions();
            unsigned int numHiddenNeurons = 10; 
            unsigned int numOutputNeurons = 1; //1 as we are using multidimensional regression
            
            //Initialize the MLP
            mlp.init(numInputNeurons, numHiddenNeurons, numOutputNeurons, Neuron::LINEAR, Neuron::SIGMOID, Neuron::SIGMOID );
            
            //Set the training settings
            mlp.setMaxNumEpochs( 1000 ); //This sets the maximum number of epochs (1 epoch is 1 complete iteration of the training data) that are allowed
            mlp.setMinChange( 1.0e-10 ); //This sets the minimum change allowed in training error between any two epochs
            mlp.setLearningRate( 0.001 ); //This sets the rate at which the learning algorithm updates the weights of the neural network
            mlp.setNumRandomTrainingIterations( 5 ); //This sets the number of times the MLP will be trained, each training iteration starts with new random values
            mlp.setUseValidationSet( true ); //This sets aside a small portiion of the training data to be used as a validation set to mitigate overfitting
            mlp.setValidationSetSize( 20 ); //Use 20% of the training data for validation during the training phase
            mlp.setRandomiseTrainingOrder( true ); //Randomize the order of the training data so that the training algorithm does not bias the training
            
            //The MLP generally works much better if the training and prediction data is first scaled to a common range (i.e. [0.0 1.0])
            mlp.enableScaling( true );
            
            pipeline_R << MultidimensionalRegression(mlp,true);
        }
            break;
        default:
            return false;
            break;
    }
    
    return true;
}

//--------------------------------------------------------------
bool ofApp::setClassifier( const int type ){
    
    int nullRejection = 0; //Change this if you want to enable nullrejection
    
    AdaBoost adaboost;
    DecisionTree dtree;
    KNN knn;
    GMM gmm;
    ANBC naiveBayes;
    MinDist minDist;
    RandomForests randomForest;
    Softmax softmax;
    SVM svm;
    
    this->classifierType = type;
    
    switch( classifierType ){
        case ADABOOST:
            adaboost.enableNullRejection( nullRejection ); // The GRT AdaBoost algorithm does not currently support null rejection, although this will be added at some point in the near future.
            adaboost.setNullRejectionCoeff( 3 );
            pipeline_C.setClassifier( adaboost );
            break;
        case DECISION_TREE:
            dtree.enableNullRejection( nullRejection );
            dtree.setNullRejectionCoeff( 3 );
            dtree.setMaxDepth( 10 );
            dtree.setMinNumSamplesPerNode( 3 );
            dtree.setRemoveFeaturesAtEachSpilt( false );
            pipeline_C.setClassifier( dtree );
            break;
        case KKN:
            knn.enableNullRejection( nullRejection );
            knn.setNullRejectionCoeff( 3 );
            pipeline_C.setClassifier( knn );
            break;
        case GAUSSIAN_MIXTURE_MODEL:
            gmm.enableNullRejection( nullRejection );
            gmm.setNullRejectionCoeff( 3 );
            pipeline_C.setClassifier( gmm );
            break;
        case NAIVE_BAYES:
            naiveBayes.enableNullRejection( nullRejection );
            naiveBayes.setNullRejectionCoeff( 3 );
            pipeline_C.setClassifier( naiveBayes );
            break;
        case MINDIST:
            minDist.enableNullRejection( nullRejection );
            minDist.setNullRejectionCoeff( 3 );
            pipeline_C.setClassifier( minDist );
            break;
        case RANDOM_FOREST_10:
            randomForest.enableNullRejection( nullRejection );
            randomForest.setNullRejectionCoeff( 3 );
            randomForest.setForestSize( 10 );
            randomForest.setNumRandomSplits( 2 );
            randomForest.setMaxDepth( 10 );
            randomForest.setMinNumSamplesPerNode( 3 );
            randomForest.setRemoveFeaturesAtEachSpilt( false );
            pipeline_C.setClassifier( randomForest );
            break;
        case RANDOM_FOREST_100:
            randomForest.enableNullRejection( nullRejection );
            randomForest.setNullRejectionCoeff( 3 );
            randomForest.setForestSize( 100 );
            randomForest.setNumRandomSplits( 2 );
            randomForest.setMaxDepth( 10 );
            randomForest.setMinNumSamplesPerNode( 3 );
            randomForest.setRemoveFeaturesAtEachSpilt( false );
            pipeline_C.setClassifier( randomForest );
            break;
        case RANDOM_FOREST_200:
            randomForest.enableNullRejection( nullRejection );
            randomForest.setNullRejectionCoeff( 3 );
            randomForest.setForestSize( 200 );
            randomForest.setNumRandomSplits( 2 );
            randomForest.setMaxDepth( 10 );
            randomForest.setMinNumSamplesPerNode( 3 );
            randomForest.setRemoveFeaturesAtEachSpilt( false );
            pipeline_C.setClassifier( randomForest );
            break;
        case SOFTMAX:
            softmax.enableNullRejection( false ); //Does not support null rejection
            softmax.setNullRejectionCoeff( 3 );
            pipeline_C.setClassifier( softmax );
            break;
        case SVM_LINEAR:
            svm.enableNullRejection( nullRejection );
            svm.setNullRejectionCoeff( 3 );
            pipeline_C.setClassifier( SVM(SVM::LINEAR_KERNEL) );
            break;
        case SVM_RBF:
            svm.enableNullRejection( nullRejection );
            svm.setNullRejectionCoeff( 3 );
            pipeline_C.setClassifier( SVM(SVM::RBF_KERNEL) );
            break;
        default:
            return false;
            break;
    }
    
    return true;
}

//--------------------------------------------------------------
float ofApp:: getGesture (Gesture gesture){
    
    //Current issues: How to make it scale accordingly?
    
    if(tracker.size()<1) {
        return 0;
    }
    int start = 0, end = 0;
    int gestureMultiplier = 10;
    
    
    switch(gesture) {
            // left to right of mouth
        case MOUTH_WIDTH: start = 48; end = 54; break;
            // top to bottom of inner mouth
        case MOUTH_HEIGHT: start = 51; end = 57; gestureMultiplier = 10; break;
            // center of the eye to middle of eyebrow
        case LEFT_EYEBROW_HEIGHT: start = 38; end = 20; gestureMultiplier = 10; break;
            // center of the eye to middle of eyebrow
        case RIGHT_EYEBROW_HEIGHT: start = 43; end = 23; gestureMultiplier = 10; break;
            // upper inner eye to lower outer eye
        case LEFT_EYE_OPENNESS: start = 38; end = 40; gestureMultiplier = 25; break;
            // upper inner eye to lower outer eye
        case RIGHT_EYE_OPENNESS: start = 43; end = 47; gestureMultiplier = 25; break;
            // nose center to chin center
        case JAW_OPENNESS: start = 33; end = 8; break;
            // left side of nose to right side of nose
        case NOSTRIL_FLARE: start = 31; end = 35; break;
    }
    
    
    //Normalized
    return (gestureMultiplier*abs(abs(tracker.getInstances()[0].getLandmarks().getImagePoint(start).getNormalized().y - tracker.getInstances()[0].getLandmarks().getImagePoint(end).getNormalized().y)));
    
    
}


//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    
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