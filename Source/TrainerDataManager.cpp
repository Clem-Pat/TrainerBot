#include "TrainerDataManager.h"
#include "Trainer.h"
#include <iostream>
#include <cmath>
#include <fstream>
#include "sqlite3.h"
#include <chrono>
#include <iomanip>
#include <sstream>

std::ofstream writing_file;



std::string GetTodaysDate() {
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

std::string TrainerDataManager::FindInDataBase(std::string username, std::string typeOfExercice, std::string TotalWeightWanted, std::string whatWeLookFor, std::string columnName) {
    // whatWeLookFor is the SQL function to use (e.g. "MAX", "MIN", "AVG", "SUM", etc.)
    // columnName is the name of the column to apply the function on (e.g. "nbreOfReps", "velocityOfSet", etc.)
    std::string query = "SELECT " + whatWeLookFor + "(" + columnName + ")" + " FROM WorkoutSession WHERE userId = (SELECT id FROM UserInfo WHERE Username = '" + username + "') AND typeOfExercise = '" + typeOfExercice + "' AND load = " + TotalWeightWanted + ";";
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

void TrainerDataManager::SaveSessionInDataBase() {
    std::cout<<"Saving session in database"<<std::endl;
    for (auto& analyzer : TheTrainer->predictor.seriesAnalyzers) {
        //Saving One RM value in database
		char* errMsg = nullptr;
        int rc;
        int id;
        std::string sql;
        if (dynamic_cast<OneRMAnalyzer*>(analyzer.get())){
            OneRMAnalyzer* OneRManalyzer = dynamic_cast<OneRMAnalyzer*>(analyzer.get());
            if (OneRManalyzer->closed) {
                std::cout << "Saving OneRM in database" << std::endl;
                id = GetUserId(OneRManalyzer->Username);
                if (id == -1) { AddUser(OneRManalyzer->Username); }
                id = GetUserId(OneRManalyzer->Username);
                if (OneRManalyzer->OneRMValue != -1){
			        sql = "INSERT INTO OneRM (userId, typeOfExercise, valueOfOneRM, date) VALUES (" + std::to_string(id) + ", '" + OneRManalyzer->TypeOfExercice + "', " + std::to_string(OneRManalyzer->OneRMValue) + ", '" + GetTodaysDate() + "');";
                    errMsg = nullptr;
                    rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
			        if (rc != SQLITE_OK) {
				        std::cout << "Failed to insert OneRM in database: " << sqlite3_errmsg(db) << " " << sql << std::endl;
				        sqlite3_free(errMsg);
                    }
                }
                if (OneRManalyzer->EnduranceValue != -1){
                    std::cout << "Saving Endurance in database" << std::endl;
                    sql = "INSERT INTO Endurance (userId, typeOfExercise, valueOfEndurance, date) VALUES (" + std::to_string(id) + ", '" + OneRManalyzer->TypeOfExercice + "', " + std::to_string(OneRManalyzer->EnduranceValue) + ", '" + GetTodaysDate() + "');";
                    errMsg = nullptr;
                    rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
                    if (rc != SQLITE_OK) {
                        std::cout << "Failed to insert Endurance in database: " << sqlite3_errmsg(db) << " " << sql << std::endl;
                        sqlite3_free(errMsg);
                    }
                }
            }
        }
        if (dynamic_cast<SetDataSaver*>(analyzer.get())) {
            SetDataSaver* dataSaver = dynamic_cast<SetDataSaver*>(analyzer.get());
            if (dataSaver->closed) {
                std::cout << "Saving WorkoutSession in database" << std::endl;
                id = GetUserId(dataSaver->Username);
                if (id == -1) { AddUser(dataSaver->Username); }
                id = GetUserId(dataSaver->Username);
                if (dataSaver->numberOfReps != 0){
                    sql = "INSERT INTO WorkoutSession (userId, typeOfExercise, load, nbreOfReps, velocitiesOfSet, coordinatesPointsProfile, usedForOneRM, date) VALUES (" + std::to_string(id) + ", '" + dataSaver->TypeOfExercice + "', " + std::to_string(dataSaver->TotalWeightWanted) + ", " + std::to_string(dataSaver->numberOfReps) + ", '" + VectorToString(dataSaver->VelocityOfSet) + "', '" + VectorToString(dataSaver->profileShape) + "', '" + std::to_string(dataSaver->SetUsedForOneRMExperiment) + "', '" + GetTodaysDate() + "');";
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
