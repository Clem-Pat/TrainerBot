#include "TrainerDataManager.h"
#include "Trainer.h"
#include <iostream>
#include <cmath>
#include <fstream>


std::ofstream writing_file;

void TrainerDataManager::Begin(const std::string& filename) {
    writing_file.open(filename.c_str(), std::ios::out);
    writing_file << "TimeStamp[ms],Theta0[rad],Theta1[rad],theta_diff0[rad],theta_diff1[rad],distance0[m],distance1[m],Ang_V0[rad/s],Ang_V1[rad/s],Ang_A0[rad/s2],Ang_A1[rad/s2],OUTPUT0[Nm],OUTPUT1[Nm],TimeStamp[ms],POWER1[kgm/s2],POWER2[kgm/s2],POWER_both[kgm/s2],POWER1[N],POWER2[N],POWER_both[N],dennryu_Left,dennryu_Right,Loadcell_Left,Loadcell_Right" << std::endl;
    std::cout << "writing " << filename << "...start" << std::endl;
}

void TrainerDataManager::AddLineToFile(float ts, double theta_deg[], double theta_diff[], double distance[]) {
    //std::cout << std::fixed;
    writing_file << ts << ", "; // �^�C���X�^���v���o��
    writing_file << theta_deg[0] << ", ";
    writing_file << theta_deg[1] << ", ";
    writing_file << theta_diff[0] << ", ";
    writing_file << theta_diff[1] << ", ";
    writing_file << distance[0] << ", ";
    writing_file << distance[1] << ", ";
}

void TrainerDataManager::AddLineToFile(char line[200], bool WithEndOfLine=true) {
    writing_file << line;
    if (WithEndOfLine) {
        writing_file << std::endl;
    }
}

void TrainerDataManager::AddData(int index, const double data) {
    //AddData for Bot Simulator

    experimentData[index].push_back(data);
    if (index != 0) {
        TheTrainer->app.Plot(index-1, experimentData[0], experimentData[index], 0, experimentDataNames[index], "lines");
    }
}

void TrainerDataManager::AddData(std::vector<double>& data)  {
    //AddData for real Bot
    if (WantMeasureRep) {
        TimeDistanceAndVelocityInRep[0].push_back(data[0]); //Add time
        TimeDistanceAndVelocityInRep[1].push_back((data[5] + data[6]) / 2); //Add measured distance
        TimeDistanceAndVelocityInRep[2].push_back((data[1] + data[2]) / 2); //Add measured angle velocity
    }
    for (int i = 0; i < data.size(); i++)
    {
        experimentData[i].push_back(data[i]);
        if (i != 0) {
            TheTrainer->app.Plot(i-1, experimentData[0], experimentData[i], TheTrainer->app.indexOfWindowToPlotRawData, experimentDataNames[i], "scatter");
        }
    }
}

int TrainerDataManager::findIndexOfEmptyData() {
    int k = 0;
    while (k < sizeof(experimentData))
    {
        if (experimentData[k].size() == 0) {
            break;
        }
        else {
            k++;
        }
    }
    return k;
}

void TrainerDataManager::ConfigureDataName(int index, const std::string& dataName) {
	experimentDataNames[index] = dataName;
}

void TrainerDataManager::ConfigureDataNames(const std::vector<std::string>& dataNames) {
    for (int i = 0; i < sizeof(dataNames); i++)
    {
        experimentDataNames[i] = dataNames[i];
    }
}

void TrainerDataManager::End() {
    writing_file.close();
    std::cout << "writing...end" << std::endl;
}
