#include "Trainer.h"
#include "SeriesAnalyzer.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <numeric> // Include this for std::accumulate
#include "Trainer.h"

void SeriesAnalyzer::Begin() {
	if (TheTrainer != nullptr) {
		this->Username = TheTrainer->Username; // We were forced to add this line because we could not initialize the TypeOfExercise in the constructor as the circular dependencies problem was hit due to including Trainer.h in the header file. 
		this->TypeOfExercise = TheTrainer->TypeOfExercise;  //Same
		this->TotalWeightWanted = TheTrainer->botManager.TotalWeightWanted; //Same
		if (TheTrainer->UsingLoadProfile){
			this->UsingLoadProfile = true;
			this->profileShape = TheTrainer->app.trapeze.vertices;
		}
	}
}

double SeriesAnalyzer::getReferencedMaxHeight() {
	std::string username = TheTrainer->Username;
	std::string typeOfExercise = TheTrainer->TypeOfExercise;
	std::vector<double> referencedMaxHeights = std::vector<double>();
	for (auto& analyzer : TheTrainer->predictor.seriesAnalyzers) {
		if (dynamic_cast<SetDataSaver*>(analyzer.get())) {
			SetDataSaver* setAnalyzer = dynamic_cast<SetDataSaver*>(analyzer.get());
			if (setAnalyzer->Username == username && setAnalyzer->TypeOfExercise == typeOfExercise) {
				if (referencedMaxHeights.size() > 5) {
					break;
				}
				else{
					for (double maxHeight : setAnalyzer->maxHeights){
						referencedMaxHeights.push_back(maxHeight);
					}
				}
			}
		}
	}
	if (referencedMaxHeights.size() == 0) return 0; // We never acquired heights so we return the default value
	return std::accumulate(referencedMaxHeights.begin(), referencedMaxHeights.end(), 0.0) / referencedMaxHeights.size();
}

void SeriesAnalyzer::closeAnalyzer(double tf) {
	this->tf = tf;
	this->duration = tf - t0;
	this->meanMaxHeight = std::accumulate(maxHeights.begin(), maxHeights.end(), 0.0) / maxHeights.size();
	this->meanMaxVelocity = std::accumulate(maxVelocities.begin(), maxVelocities.end(), 0.0) / maxVelocities.size();
	auto indexMinMaxVelocities = std::min_element(maxVelocities.begin(), maxVelocities.end());
	if (indexMinMaxVelocities != maxVelocities.end()) {
		this->minMaxVelocities = *indexMinMaxVelocities;
		this->indexOfRepOfMinMaxVelocities = std::distance(maxVelocities.begin(), indexMinMaxVelocities);
	}
	auto indexMaxVelocities = std::max_element(maxVelocities.begin(), maxVelocities.end());
	if (indexMaxVelocities != maxVelocities.end()) {
		this->maxMaxVelocities = *indexMaxVelocities;
		this->indexOfRepOfMaxMaxVelocities = std::distance(maxVelocities.begin(), indexMaxVelocities);
	}
	this->numberOfReps = maxVelocities.size();
	this->setsWithProblemsToKeep = std::vector<bool>(problemsInSet.size(), true);
	this->closed = true;
}

void InitialAnalyzer::update(bool FinishedLastSet) {
	//Recalculating the max Height Reference();
	if (!FinishedLastSet){
		if (MaxHeightsRecordedSinceLastTypeOfExerciseChange.size() < 5) {
			referencedMaxHeight = std::accumulate(MaxHeightsRecordedSinceLastTypeOfExerciseChange.begin(), MaxHeightsRecordedSinceLastTypeOfExerciseChange.end(), 0.0) / MaxHeightsRecordedSinceLastTypeOfExerciseChange.size();
		}
	}
	else {

	}
}

void SeriesAnalyzer::DoMission() {
	std::cout << "ERROR : No mission has been assigned to this SeriesAnalyzer yet." << id << " " << name << std::endl;
	// Empty, each derived class will implement its own DoMission method
}

void SetDataSaver::DoMission() {
	//The mission of those analyzers is to save the data of the set
	this->TimeOfSet.push_back(TheTrainer->app.listXToPlot[36 - 1].back()); //We save the current time 
	for (int i = 0; i < 2; i++) {
		this->HeightOfSet[i].push_back(TheTrainer->app.listYToPlot[36 + 3 * i - 1].back()); 
		this->VelocityOfSet[i].push_back(TheTrainer->app.listYToPlot[37 + 3 * i - 1].back());
		this->AccelerationOfSet[i].push_back(TheTrainer->app.listYToPlot[38 + 3 * i - 1].back());
	}
}

void CounterAnalyzer::DoMission() {
	//The mission of those analyzers is to update the amount of reps done in the app.
	int indexLastSetDataSaverAnalyzer = TheTrainer->predictor.FindLastAnalyzerOfType<SetDataSaver>();
	if (indexLastSetDataSaverAnalyzer != -1) {
		auto setAnalyzer = dynamic_cast<SetDataSaver*>(TheTrainer->predictor.seriesAnalyzers[indexLastSetDataSaverAnalyzer].get());
		if (setAnalyzer->Username == Username && setAnalyzer->TypeOfExercise == TypeOfExercise && setAnalyzer->TotalWeightWanted == TotalWeightWanted) {
			numberOfReps = setAnalyzer->numberOfReps;
			TheTrainer->app.DeleteRobotConsoleMessage("One RM according to Lander : ");
			TheTrainer->app.DeleteRobotConsoleMessage("One RM according to Epley : ");
			TheTrainer->app.DeleteRobotConsoleMessage("One RM according to Mayhew : ");
			TheTrainer->app.AddRobotConsoleMessage("One RM according to Lander : " + std::to_string(TheTrainer->predictor.Lander(TotalWeightWanted, numberOfReps)));
			TheTrainer->app.AddRobotConsoleMessage("One RM according to Epley : " + std::to_string(TheTrainer->predictor.Epley(TotalWeightWanted, numberOfReps)));
			TheTrainer->app.AddRobotConsoleMessage("One RM according to Mayhew : " + std::to_string(TheTrainer->predictor.Mayhew(TotalWeightWanted, numberOfReps)));
		}
		else {
			std::cout << "ERROR while finishing the Epley experiment, could not find a SetDataSaver with the same Username and TypeOfExercise" << std::endl;
		}
	}
	else {
		std::cout << "ERROR while finishing the Epley experiment, could not find a SetDataSaver" << std::endl;
	}
}

void OneRMAnalyzer::DoMission() {
	std::cout << "Do Mission" << std::endl;
	//The mission of those analyzers is to update the OneRM experiment data AND the endurance experiment data. As it is defined by former student Natsuo Tojo.
	auto index = TheTrainer->predictor.FindLastAnalyzerOfType<SetDataSaver>();
	if (index != -1) {
		auto setAnalyzer = dynamic_cast<SetDataSaver*>(TheTrainer->predictor.seriesAnalyzers[index].get());
		for (int i = 0; i < setAnalyzer->maxVelocities.size(); i++) {
			OneRMExperimentLoads.push_back(setAnalyzer->TotalWeightWanted);
			OneRMExperimentVelocities.push_back(setAnalyzer->maxVelocities[i]);
			EnduranceExperimentVelocitiesLoss.push_back(100 * (setAnalyzer->maxMaxVelocities - setAnalyzer->maxVelocities[i]) / setAnalyzer->maxMaxVelocities);
			EnduranceExperimentRepetitionRatio.push_back(100 * (i + 1) / setAnalyzer->maxVelocities.size());
		}
		allMinMaxVelocitiesOfExperiment.push_back(setAnalyzer->minMaxVelocities);
	}
	UpdateAndPlotOneRMExperiment();
}

void OneRMAnalyzer::UpdateAndPlotOneRMExperiment() {
	std::cout << "update and plot OneRM experiment" << std::endl;
	OneRMVelocityValue = std::accumulate(allMinMaxVelocitiesOfExperiment.begin(), allMinMaxVelocitiesOfExperiment.end(), 0.0) / allMinMaxVelocitiesOfExperiment.size();
	TheTrainer->app.Plot(55, OneRMExperimentLoads, OneRMExperimentVelocities, 1, "Max velocity during reps", "scatter");
	TheTrainer->app.Plot(56, EnduranceExperimentVelocitiesLoss, EnduranceExperimentRepetitionRatio, 1.1, "Endurance experiment", "scatter");
	if (TheTrainer->predictor.WantStopOneRMExperiment) {
		TheTrainer->predictor.AddGraphToDoRegressionOn(55, 55);
		TheTrainer->predictor.AddGraphToDoRegressionOn(56, 56);
	}
}

double OneRMAnalyzer::GetNumberOfRepAtSpecificVloss(double Vloss) {
	//Vloss is the velocity loss in % from the max velocity of the set. In the former student Natsuo Tojo's work, this function returns the N_per value
	return CoefficientsEnduranceLaw[0] * Vloss * Vloss + CoefficientsEnduranceLaw[1] * Vloss + CoefficientsEnduranceLaw[2];
}