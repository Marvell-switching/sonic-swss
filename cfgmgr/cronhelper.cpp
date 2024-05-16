#include "cronhelper.h"
#include <regex>
#include <string>
#include <stdexcept>
#include "logger.h"

using namespace std;

// CronField class implementation

// Constructor
CronField::CronField(string field, string regex):m_field(field), m_regex(regex){
    if (!this->validate()){
        throw invalid_argument("Cron field does not match regex");
    }
}

// Validate field
bool CronField::validate(){
    regex pattern(this->m_regex);
    return regex_match(this->m_field, pattern);
}

// CronExpression class implementation

// Constructor
CronExpression::CronExpression(const string &expression){
    if (!this->parse(expression)){
        throw invalid_argument("Invalid cron expression");
    }
}

// Parse expression
bool CronExpression::parse(const string &expression){

    // Tokenize expression
    vector<string> tokens;
    string token;
    istringstream tokenStream(expression);
    
    while (getline(tokenStream, token, ' ')){
        tokens.push_back(token);
    }

    // Validate number of tokens
    if (tokens.size() != 5){
        return false;
    }

    try
    {    
        // Minute
        CronField minute(tokens[0], m_cronMinuteRegex);
        this->fields.push_back(minute);

        // Hour
        CronField hour(tokens[1], m_cronHourRegex);
        this->fields.push_back(hour);

        // Day of month
        CronField dayOfMonth(tokens[2], m_cronDayOfMonthRegex);
        this->fields.push_back(dayOfMonth);

        // Month
        CronField month(tokens[3], m_cronMonthRegex);
        this->fields.push_back(month);

        // Day of week
        CronField dayOfWeek(tokens[4], m_cronDayOfWeekRegex);
        this->fields.push_back(dayOfWeek);

    }
    catch(const std::invalid_argument& e)
    {
       SWSS_LOG_ERROR(e.what());
       return false;
    }

    return true;
}

// Get fields
vector<CronField> CronExpression::getFields(){
    return this->fields;
}

// Get expression
string CronExpression::getExpression(){
    string expression = "";
    for (const auto &field : this->fields){
        expression += field.getExpression() + " ";
    }
    return expression;
}

// CronTimeRange class implementation

// Constructor
CronTimeRange::CronTimeRange(string name, CronExpression start, CronExpression end):expression(name), m_start(start), m_end(end){};

// Check if time range is active
bool CronTimeRange::isActive(){
    // Get current time
    time_t now = time(0);
    tm *ltm = localtime(&now);

    // Check if current time is within time range
    for (const auto &field : this->m_start.getFields()){
        // Check if current time is within start time range
        if (!field.validate()){
            return false;
        }
    }

    for (const auto &field : this->m_end.getFields()){
        // Check if current time is within end time range
        if (!field.validate()){
            return false;
        }
    }

    return true;
}