/*!
 * Class that implements the thread responsible for the video controller.
 *
 * \name VideoController
 * \author David Lunardi Flam
 * \version
 * \since 10/14/2008
 */
#include "VideoController.h"

VideoController::VideoController(AbstractCamera* camera, AbstractCalibrator* calibrator, AbstractPOIFinder* POIFinder,
		AbstractTracker* tracker) :
	_videoStatus(VideoStatusEnum::PLAY_LIVE), _cameraRef(camera), _calibrator(calibrator), _POIFinder(POIFinder),
			_tracker(tracker) {

	poiDetectionEndEvent = CreateEvent(NULL, NULL, TRUE, NULL);
	trackingEndEvent = CreateEvent(NULL, NULL, TRUE, NULL);
}

VideoController::~VideoController() {

	delete _calibrator;
	delete _POIFinder;
	delete _tracker;
}

void VideoController::run() {

	while (true) {

		IplImage* currentFrame = NULL;
		vector<POI> newPOIs;

		if (_videoStatus == VideoStatusEnum::PLAY_LIVE) {

			currentFrame = _cameraRef->getFrame();
			newPOIs = _POIFinder->getPOIsInImage(currentFrame);

		} else if (_videoStatus == VideoStatusEnum::RECORD) {

			//--- Record video...
			currentFrame = _cameraRef->getFrame();
			cvWriteFrame(_videoWriter, currentFrame);

			_POIsImageWidgetRef->refreshImage(currentFrame, newPOIs);

		} else if (_videoStatus == VideoStatusEnum::STOP) {

			//--- Do nothing...

		} else {
			logERROR("Unrecognized video status!");
		}

		SetEvent(poiDetectionEndEvent);
		suspend();

		if (_videoStatus == VideoStatusEnum::PLAY_LIVE) {
			//--- POIs 2d tracking
			trackNewPOIs(newPOIs, currentFrame);
		}

		SetEvent(trackingEndEvent);
		suspend();

	}
}

void VideoController::trackNewPOIs(vector<POI> detectedPOIs, IplImage* currentFrame) {

	map<string, POI> currentInitializedPOIs = _cameraRef->getPOIs();

	int radius = 50;

	//--- Tracker refreshes map of current initialized POIs with newly detected ones.
	_tracker->refreshPOIsPosition(currentInitializedPOIs, detectedPOIs, _cameraRef->getWidth() - 1,
			_cameraRef->getHeight() - 1, radius);

	// Peito � o POI de teste
	if (currentInitializedPOIs.find("Peito") != currentInitializedPOIs.end()){
		// Desenha a posi��o estimada do ponto
		cvCircle(currentFrame, cvPointFrom32f(currentInitializedPOIs["Peito"].getPredictedPosition()), 5, cvScalar(0,0,255), -1);
		// Desenha o circulo da �rea de busca
		cvCircle(currentFrame, cvPointFrom32f(currentInitializedPOIs["Peito"].getPredictedPosition()), radius, cvScalar(255,0,0));

	}


	//--- We must update cameras POIs
	_cameraRef->setPOIs(currentInitializedPOIs);

	//--- Builds display list
	vector<POI> POIsDisplayList = detectedPOIs;

	//---Insert initialized POIs into POIs display list
	for (map<string, POI>::const_iterator it = currentInitializedPOIs.begin(); it != currentInitializedPOIs.end(); ++it) {
		POIsDisplayList.push_back(it->second);
	}

	//--- Updates image
	_POIsImageWidgetRef->refreshImage(currentFrame, POIsDisplayList);
}

void VideoController::stopVideoCapture() {

	//--- If recording, stops it!
	if (_videoStatus == VideoStatusEnum::RECORD) {
		cvReleaseVideoWriter(&_videoWriter);
	}

	_videoStatus = VideoStatusEnum::STOP;
}

void VideoController::startVideoCapture() {

	_videoStatus = VideoStatusEnum::PLAY_LIVE;
}

void VideoController::startRecordingVideo(const char* videoFilePath) {

	_videoStatus = VideoStatusEnum::RECORD;

	//--- video without compression, we don't want artifacts while testing algoritms
	//_videoWriter = cvCreateVideoWriter(videoFilePath, CV_FOURCC('I', 'Y', 'U', 'V'), _cameraRef->getFrameRate(),
	//		cvSize(_cameraRef->getWidth(), _cameraRef->getHeight()));

	_videoWriter = cvCreateVideoWriter(videoFilePath,  CV_FOURCC('Y', 'U', 'V', '8'), 30.0,
				cvSize(640, 480));
}


void VideoController::restartTracker(){

	//_tracker->restartTracker();

}
