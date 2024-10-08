/*
 * Application Name: CoachBot
 * Version: 1.1.1
 * Last Modified: 2023-23-08
 * Author: Hashimoto Lab
 * Developer: Clement Patrizio / clement.patrizio@ensta-bretagne.org
 * Please refer to the Datasheet to know more about the application
 */


#pragma once
#include <string>
#include <vector>
#include "sqlite3.h"

class SetDataSaver;  //Pre-declaration to avoid circular dependencies
class OneRMAnalyser;

class Trainer;

class TrainerDataManager {
public:
    
    void Begin(const std::string& filename);
    std::string GetTodaysDate();
    int GetUserId(const std::string& username);
    int CountUsers();
    void AddUser(const std::string& username, const std::string& password = "NULL");
    void AddLineToFile(float ts, double theta_deg[], double theta_diff[], double distance[]);
    void AddLineToFile(char line[200], bool WithEndOfLine);
    void AddData(int index, const double data);
    void AddData(std::vector<double>& data, std::vector<bool> WantToPlot = std::vector<bool>(1, true));
    int findIndexOfEmptyData();
    void ConfigureDataName(int index, const std::string& dataName);
    void ConfigureDataNames(const std::vector<std::string>& dataNames);
    void UploadLastOneRMExperimentFromDataBase(std::string username, std::string typeOfExercise);
    std::string FindInDataBase(std::string username, std::string typeOfExercise, std::string TotalWeightWanted, std::string whatWeLookFor, std::string columnName);
    std::vector<OneRMAnalyser> FindOneRMExperimentInDataBase(std::string username, std::string date = "NULL", std::string typeOfExercise = "NULL");
    std::vector<SetDataSaver> FindWorkoutSessionInDataBase(std::string username, std::string date);
    void SaveSessionInDataBase();
    void SaveSetInFile();
    void End();

    int nbreOfDataToRegister = 50;
    std::vector<std::vector<double>> experimentData = std::vector<std::vector<double>>(nbreOfDataToRegister, std::vector<double>());
    std::vector<std::string> experimentDataNames = std::vector<std::string>(nbreOfDataToRegister, "");
    sqlite3* db;

    Trainer* TheTrainer;
    TrainerDataManager(Trainer* trainer) {
        this->TheTrainer = trainer;
        //experimentDataNames[0] = "X";
        //experimentDataNames[1] = "Y";
    }
};
