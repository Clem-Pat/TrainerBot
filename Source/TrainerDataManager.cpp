#include "TrainerDataManager.h"
#include "Trainer.h"
#include <iostream>
#include <cmath>
#include <fstream>
#include "sqlite3.h"


std::ofstream writing_file;

void TrainerDataManager::Begin(const std::string& filename) {
    //writing_file.open(filename.c_str(), std::ios::out);
    //writing_file << "TimeStamp[ms],Theta0[rad],Theta1[rad],theta_diff0[rad],theta_diff1[rad],distance0[m],distance1[m],Ang_V0[rad/s],Ang_V1[rad/s],Ang_A0[rad/s2],Ang_A1[rad/s2],OUTPUT0[Nm],OUTPUT1[Nm],TimeStamp[ms],POWER1[kgm/s2],POWER2[kgm/s2],POWER_both[kgm/s2],POWER1[N],POWER2[N],POWER_both[N],dennryu_Left,dennryu_Right,Loadcell_Left,Loadcell_Right" << std::endl;
    //std::cout << "writing " << filename << "...start" << std::endl;

    //TODO Failed to open database, sqlite3 is not linked apparently. 
    int rc = sqlite3_open((filename + ".db").c_str(), &db);

    if (rc) {
        std::cout << "Error opening SQLite3 database: " << sqlite3_errmsg(db) << std::endl;
    }
    else {
        std::cout << "Opened database successfully" << std::endl;
    }

    const char* createUserInfoTable = "CREATE TABLE IF NOT EXISTS UserInfo ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "Username TEXT NOT NULL,"
        "password TEXT NOT NULL);";

    const char* createOneRMTable = "CREATE TABLE IF NOT EXISTS OneRM ("
        "date TEXT NOT NULL,"
        "userId INTEGER,"
        "typeOfExercise TEXT NOT NULL,"
        "valueOfOneRM REAL NOT NULL,"
        "FOREIGN KEY(userId) REFERENCES UserInfo(id));";

    const char* createWorkoutSessionTable = "CREATE TABLE IF NOT EXISTS WorkoutSession ("
        "userId INTEGER,"
        "typeOfExercise TEXT NOT NULL,"
        "nbreOfSeries INTEGER NOT NULL,"
        "MeanNbreOfReps REAL NOT NULL,"
        "FOREIGN KEY(userId) REFERENCES UserInfo(id));";

    // Execute SQL statements
    char* errMsg = nullptr;
    rc = sqlite3_exec(db, createUserInfoTable, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cout << "Failed to create UserInfo table: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_free(errMsg);
    }

    rc = sqlite3_exec(db, createOneRMTable, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cout << "Failed to create OneRM table: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_free(errMsg);
    }

    rc = sqlite3_exec(db, createWorkoutSessionTable, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cout << "Failed to create WorkoutSession table: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_free(errMsg);
    }
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
        TheTrainer->app.listOfPlotsToKeep[index - 1] = false; //We want to be able to clear this data from the plot one day.  
    }
}

void TrainerDataManager::AddData(std::vector<double>& data, std::vector<bool> WantToPlot)  {
    //AddData for real Bot
    if (WantToPlot.size() != data.size()) {
		WantToPlot = std::vector<bool>(data.size(), true); //If nothing is specified in WantToPlot argument it means we want to plot everything
        if (WantToPlot.size() != 1) { //We tried to sepcify what data to plot but it was not the same size as the data so there is a human error, we let the user know
			std::cout << " -- WARNING in AddData: WantToPlot.size() != data.size() so we plot every data anyway -- " << std::endl;
		}
	}
    for (int i = 0; i < data.size(); i++)
    {
        experimentData[i].push_back(data[i]);
        if (i != 0 && WantToPlot[i]) {
            TheTrainer->app.listOfPlotsToKeep[i-1] = false; //We want to be able to clear this data from the plot one day.  
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
    if (!dataNames.empty()) {
        for (int i = 0; i < dataNames.size(); i++)
        {
            experimentDataNames[i] = dataNames[i];
        }
    }
}

void TrainerDataManager::End() {
    writing_file.close();
    std::cout << "writing...end" << std::endl;
    sqlite3_close(db);
}
