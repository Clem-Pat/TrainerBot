#include "TrainerDataManager.h"
#include "Trainer.h"
#include "TrainerAnalyser.h"
#include <iostream>
#include <cmath>
#include <fstream>
#include "sqlite3.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>


std::ofstream writing_file;



std::string TrainerDataManager::GetTodaysDate() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::ostringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d"); //Date is in Year-month-day format
    return ss.str();
}

std::string VectorToString(const std::vector<std::vector<double>>& vecOfVec) {
    std::ostringstream oss;
    for (const auto& vec : vecOfVec) {
        oss << "[";
        for (const auto& val : vec) {
            oss << val << ", ";
        }
        // Remove the last comma and space if the inner vector is not empty
        if (!vec.empty()) {
            auto str = oss.str();
            oss.str("");
            oss << str.substr(0, str.length() - 2);
        }
        oss << "], ";
    }
    // Remove the last comma and space if the outer vector is not empty
    if (!vecOfVec.empty()) {
        auto str = oss.str();
        oss.str("");
        oss << str.substr(0, str.length() - 2);
    }
    return oss.str();
}

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
        "userId INTEGER,"
        "typeOfExercise TEXT NOT NULL,"
        "valueOfOneRM REAL NOT NULL,"
        "date TEXT NOT NULL,"
        "FOREIGN KEY(userId) REFERENCES UserInfo(id));";

    const char* createEnduranceTable = "CREATE TABLE IF NOT EXISTS Endurance ("
        "userId INTEGER,"
        "typeOfExercise TEXT NOT NULL,"
        "valueOfEndurance REAL NOT NULL,"
        "date TEXT NOT NULL,"
        "FOREIGN KEY(userId) REFERENCES UserInfo(id));";

    const char* createWorkoutSessionTable = "CREATE TABLE IF NOT EXISTS WorkoutSession ("
        "userId INTEGER,"
        "typeOfExercise TEXT NOT NULL,"
        "load REAL NOT NULL,"
        "nbreOfReps REAL NOT NULL,"
        "velocitiesOfSet TEXT,"
        "coordinatesPointsProfile TEXT,"
        "usedForOneRM TEXT,"
        "date TEXT NOT NULL,"
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

    rc = sqlite3_exec(db, createEnduranceTable, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cout << "Failed to create Endurance table: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_free(errMsg);
    }

    rc = sqlite3_exec(db, createWorkoutSessionTable, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cout << "Failed to create WorkoutSession table: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_free(errMsg);
    }
}

int TrainerDataManager::GetUserId(const std::string& username) {
    const char* sql = "SELECT id FROM UserInfo WHERE Username = ?";
    sqlite3_stmt* stmt = nullptr;
    int userId = -1; // Default to -1 to indicate not found

    // Prepare the SQL statement
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        // Bind the username to the query
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

        // Execute the query and check if we got a result
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            // Extract the user ID from the query result
            userId = sqlite3_column_int(stmt, 0);
        }

        // Finalize the prepared statement to free resources
        sqlite3_finalize(stmt);
    }
    else {
        std::cout << "ERROR Failed to prepare query: " << sqlite3_errmsg(db) << std::endl;
    }
    return userId;
}

int TrainerDataManager::CountUsers() {
    const char* sql = "SELECT COUNT(*) FROM UserInfo;";
    sqlite3_stmt* stmt = nullptr;
    int count = 0; // Default to 0 if no rows found or in case of error

    // Prepare the SQL statement
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        // Execute the query
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            // Extract the count from the query result
            count = sqlite3_column_int(stmt, 0);
        }
        // Finalize the prepared statement to free resources
        sqlite3_finalize(stmt);
    }
    else {
        std::cout << "ERROR Failed to prepare query: " << sqlite3_errmsg(db) << std::endl;
    }
    return count;
}

void TrainerDataManager::AddUser(const std::string& username, const std::string& password) {
    char* errMsg = nullptr;
    int id = CountUsers() + 1;
    std::string sql = "INSERT INTO UserInfo (id, Username, password) VALUES ("
        + std::to_string(id) + ", '"
        + username + "', '"
        + password + "');";

    int rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cout << "Failed to insert new user into database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_free(errMsg);
    }
    else {
        std::cout << "New user added successfully" << std::endl;
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
            TheTrainer->app.Plot(i-1, experimentData[0], experimentData[i], 0, experimentDataNames[i], "scatter");
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

std::string TrainerDataManager::FindInDataBase(std::string username, std::string typeOfExercise, std::string TotalWeightWanted, std::string whatWeLookFor, std::string columnName) {
    // whatWeLookFor is the SQL function to use (e.g. "MAX", "MIN", "AVG", "SUM", etc.)
    // columnName is the name of the column to apply the function on (e.g. "nbreOfReps", "velocityOfSet", etc.)
    std::string query = "SELECT " + whatWeLookFor + "(" + columnName + ")" + " FROM WorkoutSession WHERE userId = (SELECT id FROM UserInfo WHERE Username = '" + username + "') AND typeOfExercise = '" + typeOfExercise + "' AND load = " + TotalWeightWanted + ";";
    const char* sql = query.c_str();

    sqlite3_stmt* stmt = nullptr;
	std::string result = "NULL"; // Default to NULL if no rows found or in case of error

	// Prepare the SQL statement
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
		// Execute the query
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            if (text) {
                result = std::string(text);
            }
            else {
                result = "NULL"; // Handle null case appropriately
            }
        }
		// Finalize the prepared statement to free resources
		sqlite3_finalize(stmt);
	}
	else {
		std::cout << "ERROR Failed to prepare query " << whatWeLookFor + "(" + columnName + ") : " << sqlite3_errmsg(db) << std::endl;
	}
	return result;
}

std::vector<double> extractVelocities(const std::string& velocitiesOfSetStr) {
    std::vector<double> velocitiesOfSet;
    size_t start = 0;
    size_t end = velocitiesOfSetStr.find(',');

    auto trim = [](std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
            }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
            }).base(), s.end());
        };

    try {
        // Remove the surrounding brackets
        std::string trimmedStr = velocitiesOfSetStr.substr(1, velocitiesOfSetStr.length() - 2);

        while (end != std::string::npos) {
            std::string token = trimmedStr.substr(start, end - start);
            trim(token);
            if (token[0] == '[') { token = token.substr(1); }
            if (token.back() == ']') { token = token.substr(0, token.length() - 1); }
            velocitiesOfSet.push_back(std::stod(token));
            start = end + 1;
            end = trimmedStr.find(',', start);
        }
        std::string token = trimmedStr.substr(start);
        trim(token);
        velocitiesOfSet.push_back(std::stod(token));
    }
    catch (const std::invalid_argument& e) {
        std::cerr << "Invalid argument in extractVelocities: " << e.what() << std::endl;
    }
    catch (const std::out_of_range& e) {
        std::cerr << "Out of range in extractVelocities: " << e.what() << std::endl;
    }

    return velocitiesOfSet;
}

std::vector<std::vector<double>> extractCoordinates(const std::string& coordinatesPointsProfileStr) {
    //Extract the coordinates points profile from the string given by the SQL query
    std::vector<std::vector<double>> coordinatesPointsProfile;
    std::string coordinatesStrCopy = coordinatesPointsProfileStr; // Create a copy of the input string
    coordinatesStrCopy.erase(std::remove(coordinatesStrCopy.begin(), coordinatesStrCopy.end(), ' '), coordinatesStrCopy.end());

    if (coordinatesStrCopy[0] == '[' && coordinatesStrCopy[1] == '[') { // this means it is in the form of "[[0.0, 0.0], [0.0, 0.0]]" so we delete the surrounding brackets to "[0.0, 0.0], [0.0, 0.0]"
        coordinatesStrCopy = coordinatesStrCopy.substr(1, coordinatesStrCopy.length() - 2);
	}
    if (coordinatesStrCopy.front() == '[' && coordinatesStrCopy.back() == ']') {    //Now, we move from "[0.0, 0.0], [0.0, 0.0]" to "0.0, 0.0], [0.0, 0.0"
        coordinatesStrCopy = coordinatesStrCopy.substr(1, coordinatesStrCopy.length() - 2);
    }

    size_t start = 0;
    size_t end = coordinatesStrCopy.find("],[");

    while (end != std::string::npos) {
        std::string innerVectorStr = coordinatesStrCopy.substr(start, end - start);
        std::vector<double> innerVector;
        size_t innerStart = 0;
        size_t innerEnd = innerVectorStr.find(',');

        while (innerEnd != std::string::npos) {
            std::string token = innerVectorStr.substr(innerStart, innerEnd - innerStart);
            innerVector.push_back(std::stod(token));
            innerStart = innerEnd + 1;
            innerEnd = innerVectorStr.find(',', innerStart);
        }
        std::string token = innerVectorStr.substr(innerStart);
        innerVector.push_back(std::stod(token));
        coordinatesPointsProfile.push_back(innerVector);

        start = end + 3; // Move past the closing bracket and comma
        end = coordinatesStrCopy.find("],[", start);
    }

    // Handle the last inner vector
    std::string innerVectorStr = coordinatesStrCopy.substr(start);
    std::vector<double> innerVector;
    size_t innerStart = 0;
    size_t innerEnd = innerVectorStr.find(',');

    while (innerEnd != std::string::npos) {
        std::string token = innerVectorStr.substr(innerStart, innerEnd - innerStart);
        innerVector.push_back(std::stod(token));
        innerStart = innerEnd + 1;
        innerEnd = innerVectorStr.find(',', innerStart);
    }
    std::string token = innerVectorStr.substr(innerStart);
    innerVector.push_back(std::stod(token));
    coordinatesPointsProfile.push_back(innerVector);

    return coordinatesPointsProfile;
}

std::vector<SetDataSaver> TrainerDataManager::FindWorkoutSessionInDataBase(std::string username, std::string date) {
    //std::string query = "SELECT * FROM WorkoutSession WHERE userId = (SELECT id FROM UserInfo WHERE Username = '" + username + "') AND date = '" + date + "';";
    std::string query = "SELECT typeOfExercise, load, nbreOfReps, velocitiesOfSet, coordinatesPointsProfile, usedForOneRM FROM WorkoutSession WHERE userId = (SELECT id FROM UserInfo WHERE Username = \"" + username + "\") AND date = \"" + date + "\";";
    const char* sql = query.c_str();
    sqlite3_stmt* stmt = nullptr;
    vector<SetDataSaver> setsDataSavers = {};

    // Prepare the SQL statement
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        // Execute the query and process the results
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            // Extract the values from the query result
            std::string typeOfExercise(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            double load = sqlite3_column_double(stmt, 1);
            int nbreOfReps = sqlite3_column_int(stmt, 2);
            std::string velocitiesOfSetStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            std::vector<double> velocitiesOfSet = extractVelocities(velocitiesOfSetStr);
            std::string coordinatesPointsProfileStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
            std::vector<std::vector<double>> coordinatesPointsProfile = extractCoordinates(coordinatesPointsProfileStr);
            bool usedForOneRM = sqlite3_column_int(stmt, 5);
            bool UsingLoadProfile = coordinatesPointsProfile == std::vector<std::vector<double>>(2, std::vector<double>(2, 0.0)) ? false : true;
            
            //We create a summary SetDataSaver of the set 
            SetDataSaver setAnalyser(nullptr, 0, 0);
            setAnalyser.Username = username;
            setAnalyser.TypeOfExercise = typeOfExercise;
            setAnalyser.VelocityOfSet[0] = velocitiesOfSet;
            setAnalyser.profileShape = coordinatesPointsProfile;
            setAnalyser.UsingLoadProfile = coordinatesPointsProfile == std::vector<std::vector<double>>(1, std::vector<double>(1, 0.0)) ? false : true;
            setAnalyser.SetUsedForOneRMExperiment = usedForOneRM;
            setAnalyser.numberOfReps = nbreOfReps;
            setAnalyser.TotalWeightWanted = load;
            setsDataSavers.push_back(setAnalyser);
        }

        // Finalize the prepared statement to free resources
        sqlite3_finalize(stmt);
    }
    else {
        std::cout << "ERROR Failed to prepare query: " << sqlite3_errmsg(db) << std::endl;
    }
    return setsDataSavers;
}

void TrainerDataManager::SaveSessionInDataBase() {
    std::cout<<"Saving session in database"<<std::endl;
    for (auto& analyser : TheTrainer->predictor.sessionAnalysers) {
        //Saving One RM value in database
		char* errMsg = nullptr;
        int rc;
        int id;
        std::string sql;
        if (dynamic_cast<OneRMAnalyser*>(analyser.get())){
            OneRMAnalyser* OneRManalyser = dynamic_cast<OneRMAnalyser*>(analyser.get());
            if (OneRManalyser->closed) {
                std::cout << "Saving OneRM in database" << std::endl;
                id = GetUserId(OneRManalyser->Username);
                if (id == -1) { AddUser(OneRManalyser->Username); }
                id = GetUserId(OneRManalyser->Username);
                if (OneRManalyser->OneRMValue != -1){
			        sql = "INSERT INTO OneRM (userId, typeOfExercise, valueOfOneRM, date) VALUES (" + std::to_string(id) + ", '" + OneRManalyser->TypeOfExercise + "', " + std::to_string(OneRManalyser->OneRMValue) + ", '" + GetTodaysDate() + "');";
                    errMsg = nullptr;
                    rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
			        if (rc != SQLITE_OK) {
				        std::cout << "Failed to insert OneRM in database: " << sqlite3_errmsg(db) << " " << sql << std::endl;
				        sqlite3_free(errMsg);
                    }
                }
                if (OneRManalyser->EnduranceValue != -1){
                    std::cout << "Saving Endurance in database" << std::endl;
                    sql = "INSERT INTO Endurance (userId, typeOfExercise, valueOfEndurance, date) VALUES (" + std::to_string(id) + ", '" + OneRManalyser->TypeOfExercise + "', " + std::to_string(OneRManalyser->EnduranceValue) + ", '" + GetTodaysDate() + "');";
                    errMsg = nullptr;
                    rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
                    if (rc != SQLITE_OK) {
                        std::cout << "Failed to insert Endurance in database: " << sqlite3_errmsg(db) << " " << sql << std::endl;
                        sqlite3_free(errMsg);
                    }
                }
            }
        }
        if (dynamic_cast<SetDataSaver*>(analyser.get())) {
            SetDataSaver* dataSaver = dynamic_cast<SetDataSaver*>(analyser.get());
            if (dataSaver->closed) {
                std::cout << "Saving WorkoutSession in database" << std::endl;
                id = GetUserId(dataSaver->Username);
                if (id == -1) { AddUser(dataSaver->Username); }
                id = GetUserId(dataSaver->Username);
                if (dataSaver->numberOfReps != 0){
                    sql = "INSERT INTO WorkoutSession (userId, typeOfExercise, load, nbreOfReps, velocitiesOfSet, coordinatesPointsProfile, usedForOneRM, date) VALUES (" + std::to_string(id) + ", '" + dataSaver->TypeOfExercise + "', " + std::to_string(dataSaver->TotalWeightWanted) + ", " + std::to_string(dataSaver->numberOfReps) + ", '" + VectorToString(dataSaver->VelocityOfSet) + "', '" + VectorToString(dataSaver->profileShape) + "', '" + std::to_string(dataSaver->SetUsedForOneRMExperiment) + "', '" + GetTodaysDate() + "');";
                    errMsg = nullptr;
                    rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
                    if (rc != SQLITE_OK) {
                        std::cout << "Failed to insert WorkoutSession in database: " << sqlite3_errmsg(db) << " " << sql << std::endl;
                        sqlite3_free(errMsg);
                    }
                }
            }
        }
    }
}

void TrainerDataManager::End() {
    writing_file.close();
    std::cout << "writing...end" << std::endl;
    sqlite3_close(db);
}
