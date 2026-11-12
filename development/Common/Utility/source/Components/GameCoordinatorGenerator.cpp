#include "Components/GameCoordinatorGenerator.h"


//--------------------------------------------------------------------------------------------------
std::string yaget::comp::db::CreateUserTriggerStatement(const std::string& tableName, const std::string& itemPatternFunctionName, const std::string& action)
{
    std::string triggerType;
    std::string idField;

    if (CompareI(action, "add"))
    {
        triggerType = "INSERT";
        idField = "NEW.Id";
    }
    else if (CompareI(action, "remove"))
    {
        triggerType = "DELETE";
        idField = "OLD.Id";
    }

    auto sqlCommand = std::format("CREATE TRIGGER ItemPattern{}{} AFTER {} ON {} BEGIN INSERT OR REPLACE INTO ItemPattern (Id, Pattern) VALUES ({}, {}('{}', '{}', {})); END;",
        action,
        tableName,
        triggerType,
        tableName,
        idField,
        itemPatternFunctionName,
        tableName,
        conv::ToUpper(action),
        idField);

    return sqlCommand;
}
