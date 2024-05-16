#ifndef __CRONHELPER_H__
#define __CRONHELPER_H__

#include <string>
#include <vector>

class CronField{
    public:
        CronField(std::string field, std::string regex);

    private:
        std::string m_field;
        std::string m_regex;
        bool validate();
};

class CronExpression{
    public:
        std::vector<CronField> fields;
        
        CronExpression(const std::string &expression);
        const std::vector<CronField>& getFields() const;
        std::string getExpression();
    
    private:
        // Taken from https://www.codeproject.com/Tips/5299523/Regex-for-Cron-Expressions
        std::string m_cronMinuteRegex = R"((\*|(?:\*|(?:[0-9]|(?:[1-5][0-9])))\/(?:[0-9]|(?:[1-5][0-9]))|(?:[0-9]|(?:[1-5][0-9]))(?:(?:\-[0-9]|\-(?:[1-5][0-9]))?|(?:\,(?:[0-9]|(?:[1-5][0-9])))*)))";
        std::string m_cronHourRegex = R"((\*|(?:\*|(?:\*|(?:[0-9]|1[0-9]|2[0-3])))\/(?:[0-9]|1[0-9]|2[0-3])|(?:[0-9]|1[0-9]|2[0-3])(?:(?:\-(?:[0-9]|1[0-9]|2[0-3]))?|(?:\,(?:[0-9]|1[0-9]|2[0-3]))*)))";
        std::string m_cronDayOfMonthRegex = R"((\*|\?|L(?:W|\-(?:[1-9]|(?:[12][0-9])|3[01]))?|(?:[1-9]|(?:[12][0-9])|3[01])(?:W|\/(?:[1-9]|(?:[12][0-9])|3[01]))?|(?:[1-9]|(?:[12][0-9])|3[01])(?:(?:\-(?:[1-9]|(?:[12][0-9])|3[01]))?|(?:\,(?:[1-9]|(?:[12][0-9])|3[01]))*)))";
        std::string m_cronMonthRegex = R"((\*|(?:[1-9]|1[012]|JAN|FEB|MAR|APR|MAY|JUN|JUL|AUG|SEP|OCT|NOV|DEC)(?:(?:\-(?:[1-9]|1[012]|JAN|FEB|MAR|APR|MAY|JUN|JUL|AUG|SEP|OCT|NOV|DEC))?|(?:\,(?:[1-9]|1[012]|JAN|FEB|MAR|APR|MAY|JUN|JUL|AUG|SEP|OCT|NOV|DEC))*)))";
        std::string m_cronDayOfWeekRegex = R"((\*|\?|[0-6](?:L|\#[1-5])?|(?:[0-6]|SUN|MON|TUE|WED|THU|FRI|SAT)(?:(?:\-(?:[0-6]|SUN|MON|TUE|WED|THU|FRI|SAT))?|(?:\,(?:[0-6]|SUN|MON|TUE|WED|THU|FRI|SAT))*)))";


        bool validate();
        bool parse(const std::string &expression);
};

class CronTimeRange{
    public:
        std::string expression;
        CronTimeRange(std::string name, CronExpression start, CronExpression end);
        bool isActive();
        
    
    private:
        CronExpression m_start;
        CronExpression m_end;
};

#endif /* __CRONHELPER_H__ */