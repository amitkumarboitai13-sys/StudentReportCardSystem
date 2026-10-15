
#include <drogon/drogon.h>
#include <json/json.h>

#include <iostream>
#include <string>
#include <memory>
#include <functional>

// Calculate grade using the existing grading scale
std::string calculateGrade(double average)
{
    if (average >= 90) return "O";
    if (average >= 80) return "E";
    if (average >= 70) return "A";
    if (average >= 60) return "B";
    if (average >= 50) return "C";
    if (average >= 40) return "D";
    return "F";
}

int main()
{
    try
    {
        std::cout << "STEP 1: Program started" << std::endl;

        // Load server and MySQL configuration
       drogon::app().loadConfigFile("config.json");

        std::cout << "STEP 2: Configuration loaded" << std::endl;

        // --------------------------------------------------
        // HOME API
        // GET /
        // --------------------------------------------------
        drogon::app().registerHandler(
            "/",
            [](const drogon::HttpRequestPtr&,
                std::function<void(
                    const drogon::HttpResponsePtr&)>&& callback)
            {
                auto response =
                    drogon::HttpResponse::newHttpResponse();

                response->setBody(
                    "Student Report Card System - Server Running!");

                callback(response);
            },
            { drogon::Get }
        );

        // --------------------------------------------------
        // DATABASE TEST API
        // GET /api/db-test
        // --------------------------------------------------
        drogon::app().registerHandler(
            "/api/db-test",
            [](const drogon::HttpRequestPtr&,
                std::function<void(
                    const drogon::HttpResponsePtr&)>&& callback)
            {
                auto responseCallback =
                    std::make_shared<
                    std::function<void(
                        const drogon::HttpResponsePtr&)>
                    >(std::move(callback));

                auto dbClient =
                    drogon::app().getDbClient("default");

                dbClient->execSqlAsync(
                    "SELECT DATABASE() AS db_name",

                    [responseCallback](
                        const drogon::orm::Result& result)
                    {
                        Json::Value json;

                        json["message"] = "DATABASE CONNECTED";
                        json["database"] =
                            result[0]["db_name"].as<std::string>();

                        auto response =
                            drogon::HttpResponse::newHttpJsonResponse(
                                json);

                        (*responseCallback)(response);
                    },

                    [responseCallback](
                        const drogon::orm::DrogonDbException& e)
                    {
                        Json::Value json;
                        json["error"] = e.base().what();

                        auto response =
                            drogon::HttpResponse::newHttpJsonResponse(
                                json);

                        response->setStatusCode(
                            drogon::k500InternalServerError);

                        (*responseCallback)(response);
                    }
                );
            },
            { drogon::Get }
        );

        // --------------------------------------------------
        // GET ALL STUDENTS
        // GET /api/students
        // --------------------------------------------------
        drogon::app().registerHandler(
            "/api/students",
            [](const drogon::HttpRequestPtr&,
                std::function<void(
                    const drogon::HttpResponsePtr&)>&& callback)
            {
                auto responseCallback =
                    std::make_shared<
                    std::function<void(
                        const drogon::HttpResponsePtr&)>
                    >(std::move(callback));

                auto dbClient =
                    drogon::app().getDbClient("default");

                dbClient->execSqlAsync(
                    "SELECT roll_no, name, grade, average "
                    "FROM student ORDER BY roll_no",

                    [responseCallback](
                        const drogon::orm::Result& result)
                    {
                        Json::Value students(Json::arrayValue);

                        for (const auto& row : result)
                        {
                            Json::Value student;

                            student["roll_no"] =
                                Json::Int64(
                                    row["roll_no"].as<long long>());

                            student["name"] =
                                row["name"].as<std::string>();

                            student["grade"] =
                                row["grade"].isNull()
                                ? ""
                                : row["grade"].as<std::string>();

                            if (row["average"].isNull())
                            {
                                student["average"] = Json::Value();
                            }
                            else
                            {
                                student["average"] =
                                    row["average"].as<double>();
                            }

                            students.append(student);
                        }

                        auto response =
                            drogon::HttpResponse::
                            newHttpJsonResponse(students);

                        (*responseCallback)(response);
                    },

                    [responseCallback](
                        const drogon::orm::DrogonDbException& e)
                    {
                        Json::Value error;
                        error["error"] = e.base().what();

                        auto response =
                            drogon::HttpResponse::
                            newHttpJsonResponse(error);

                        response->setStatusCode(
                            drogon::k500InternalServerError);

                        (*responseCallback)(response);
                    }
                );
            },
            { drogon::Get }
        );

        // --------------------------------------------------
        // GET ONE STUDENT WITH SUBJECT MARKS
        // GET /api/students/{roll_no}
        // --------------------------------------------------
        drogon::app().registerHandler(
            "/api/students/{1}",
            [](const drogon::HttpRequestPtr&,
                std::function<void(
                    const drogon::HttpResponsePtr&)>&& callback,
                long long rollNo)
            {
                auto responseCallback =
                    std::make_shared<
                    std::function<void(
                        const drogon::HttpResponsePtr&)>
                    >(std::move(callback));

                auto dbClient =
                    drogon::app().getDbClient("default");

                dbClient->execSqlAsync(
                    "SELECT s.roll_no, s.name, s.grade, s.average, "
                    "m.subject_name, m.marks "
                    "FROM student s "
                    "LEFT JOIN student_marks m "
                    "ON s.roll_no = m.roll_no "
                    "WHERE s.roll_no = ? "
                    "ORDER BY m.id",

                    [responseCallback](
                        const drogon::orm::Result& result)
                    {
                        if (result.empty())
                        {
                            Json::Value error;
                            error["error"] = "Student not found";

                            auto response =
                                drogon::HttpResponse::
                                newHttpJsonResponse(error);

                            response->setStatusCode(
                                drogon::k404NotFound);

                            (*responseCallback)(response);
                            return;
                        }

                        Json::Value student;
                        Json::Value marks(Json::arrayValue);

                        student["roll_no"] =
                            Json::Int64(
                                result[0]["roll_no"].as<long long>());

                        student["name"] =
                            result[0]["name"].as<std::string>();

                        student["grade"] =
                            result[0]["grade"].isNull()
                            ? ""
                            : result[0]["grade"].as<std::string>();

                        if (result[0]["average"].isNull())
                        {
                            student["average"] = Json::Value();
                        }
                        else
                        {
                            student["average"] =
                                result[0]["average"].as<double>();
                        }

                        for (const auto& row : result)
                        {
                            if (!row["subject_name"].isNull())
                            {
                                Json::Value mark;

                                mark["subject_name"] =
                                    row["subject_name"].
                                    as<std::string>();

                                mark["marks"] =
                                    row["marks"].as<int>();

                                marks.append(mark);
                            }
                        }

                        student["marks"] = marks;

                        auto response =
                            drogon::HttpResponse::
                            newHttpJsonResponse(student);

                        (*responseCallback)(response);
                    },

                    [responseCallback](
                        const drogon::orm::DrogonDbException& e)
                    {
                        Json::Value error;
                        error["error"] = e.base().what();

                        auto response =
                            drogon::HttpResponse::
                            newHttpJsonResponse(error);

                        response->setStatusCode(
                            drogon::k500InternalServerError);

                        (*responseCallback)(response);
                    },

                    rollNo
                );
            },
            { drogon::Get }
        );

        // --------------------------------------------------
        // ADD STUDENT
        // POST /api/students
        // JSON: {"roll_no":4068,"name":"Rahul"}
        // --------------------------------------------------
        drogon::app().registerHandler(
            "/api/students",
            [](const drogon::HttpRequestPtr& req,
                std::function<void(
                    const drogon::HttpResponsePtr&)>&& callback)
            {
                auto responseCallback =
                    std::make_shared<
                    std::function<void(
                        const drogon::HttpResponsePtr&)>
                    >(std::move(callback));

                auto json = req->getJsonObject();

                if (!json ||
                    !json->isMember("roll_no") ||
                    !json->isMember("name") ||
                    !(*json)["roll_no"].isIntegral() ||
                    !(*json)["name"].isString())
                {
                    Json::Value error;
                    error["error"] =
                        "A numeric roll_no and string name are required";

                    auto response =
                        drogon::HttpResponse::
                        newHttpJsonResponse(error);

                    response->setStatusCode(
                        drogon::k400BadRequest);

                    (*responseCallback)(response);
                    return;
                }

                long long rollNo =
                    (*json)["roll_no"].asInt64();

                std::string name =
                    (*json)["name"].asString();

                if (rollNo <= 0 || name.empty())
                {
                    Json::Value error;
                    error["error"] =
                        "Invalid roll_no or name";

                    auto response =
                        drogon::HttpResponse::
                        newHttpJsonResponse(error);

                    response->setStatusCode(
                        drogon::k400BadRequest);

                    (*responseCallback)(response);
                    return;
                }

                auto dbClient =
                    drogon::app().getDbClient("default");

                dbClient->execSqlAsync(
                    "INSERT INTO student "
                    "(roll_no, name, grade, average) "
                    "VALUES (?, ?, NULL, NULL)",

                    [responseCallback, rollNo, name](
                        const drogon::orm::Result&)
                    {
                        Json::Value result;
                        result["message"] =
                            "Student added successfully";
                        result["roll_no"] = Json::Int64(rollNo);
                        result["name"] = name;

                        auto response =
                            drogon::HttpResponse::
                            newHttpJsonResponse(result);

                        response->setStatusCode(
                            drogon::k201Created);

                        (*responseCallback)(response);
                    },

                    [responseCallback](
                        const drogon::orm::DrogonDbException& e)
                    {
                        Json::Value error;
                        error["error"] = e.base().what();

                        auto response =
                            drogon::HttpResponse::
                            newHttpJsonResponse(error);

                        response->setStatusCode(
                            drogon::k500InternalServerError);

                        (*responseCallback)(response);
                    },

                    rollNo,
                    name
                );
            },
            { drogon::Post }
        );

        // --------------------------------------------------
        // ADD SUBJECT MARKS AND RECALCULATE REPORT CARD
        // POST /api/students/{roll_no}/marks
        // JSON: {"subject_name":"Math","marks":90}
        // --------------------------------------------------
        drogon::app().registerHandler(
            "/api/students/{1}/marks",
            [](const drogon::HttpRequestPtr& req,
                std::function<void(
                    const drogon::HttpResponsePtr&)>&& callback,
                long long rollNo)
            {
                auto responseCallback =
                    std::make_shared<
                    std::function<void(
                        const drogon::HttpResponsePtr&)>
                    >(std::move(callback));

                auto json = req->getJsonObject();

                if (!json ||
                    !json->isMember("subject_name") ||
                    !json->isMember("marks") ||
                    !(*json)["subject_name"].isString() ||
                    !(*json)["marks"].isIntegral())
                {
                    Json::Value error;
                    error["error"] =
                        "A string subject_name and integer marks are required";

                    auto response =
                        drogon::HttpResponse::
                        newHttpJsonResponse(error);

                    response->setStatusCode(
                        drogon::k400BadRequest);

                    (*responseCallback)(response);
                    return;
                }

                std::string subjectName =
                    (*json)["subject_name"].asString();

                int marks = (*json)["marks"].asInt();

                if (rollNo <= 0 ||
                    subjectName.empty() ||
                    marks < 0 ||
                    marks > 100)
                {
                    Json::Value error;
                    error["error"] =
                        "Invalid roll number, subject name, or marks (0-100)";

                    auto response =
                        drogon::HttpResponse::
                        newHttpJsonResponse(error);

                    response->setStatusCode(
                        drogon::k400BadRequest);

                    (*responseCallback)(response);
                    return;
                }

                auto dbClient =
                    drogon::app().getDbClient("default");

                // First, insert the subject marks
                dbClient->execSqlAsync(
                    "INSERT INTO student_marks "
                    "(roll_no, subject_name, marks) "
                    "VALUES (?, ?, ?)",

                    [responseCallback, dbClient, rollNo,
                    subjectName, marks](
                        const drogon::orm::Result&)
                    {
                        // Next, calculate the average of all marks
                        dbClient->execSqlAsync(
                            "SELECT AVG(marks) AS average "
                            "FROM student_marks WHERE roll_no = ?",

                            [responseCallback, dbClient, rollNo,
                            subjectName, marks](
                                const drogon::orm::Result& result)
                            {
                                if (result.empty() ||
                                    result[0]["average"].isNull())
                                {
                                    Json::Value error;
                                    error["error"] =
                                        "Could not calculate average";

                                    auto response =
                                        drogon::HttpResponse::
                                        newHttpJsonResponse(error);

                                    response->setStatusCode(
                                        drogon::k500InternalServerError);

                                    (*responseCallback)(response);
                                    return;
                                }

                                double average =
                                    result[0]["average"].as<double>();

                                std::string grade =
                                    calculateGrade(average);

                                // Save the calculated result
                                dbClient->execSqlAsync(
                                    "UPDATE student "
                                    "SET average = ?, grade = ? "
                                    "WHERE roll_no = ?",

                                    [responseCallback, rollNo,
                                    subjectName, marks, average,
                                    grade](
                                        const drogon::orm::Result&)
                                    {
                                        Json::Value result;
                                        result["message"] =
                                            "Marks added and report card updated";
                                        result["roll_no"] =
                                            Json::Int64(rollNo);
                                        result["subject_name"] =
                                            subjectName;
                                        result["marks"] = marks;
                                        result["average"] = average;
                                        result["grade"] = grade;

                                        auto response =
                                            drogon::HttpResponse::
                                            newHttpJsonResponse(result);

                                        response->setStatusCode(
                                            drogon::k201Created);

                                        (*responseCallback)(response);
                                    },

                                    [responseCallback](
                                        const drogon::orm::
                                        DrogonDbException& e)
                                    {
                                        Json::Value error;
                                        error["error"] = e.base().what();

                                        auto response =
                                            drogon::HttpResponse::
                                            newHttpJsonResponse(error);

                                        response->setStatusCode(
                                            drogon::k500InternalServerError);

                                        (*responseCallback)(response);
                                    },

                                    average,
                                    grade,
                                    rollNo
                                );
                            },

                            [responseCallback](
                                const drogon::orm::DrogonDbException& e)
                            {
                                Json::Value error;
                                error["error"] = e.base().what();

                                auto response =
                                    drogon::HttpResponse::
                                    newHttpJsonResponse(error);

                                response->setStatusCode(
                                    drogon::k500InternalServerError);

                                (*responseCallback)(response);
                            },

                            rollNo
                        );
                    },

                    [responseCallback](
                        const drogon::orm::DrogonDbException& e)
                    {
                        Json::Value error;
                        error["error"] = e.base().what();

                        auto response =
                            drogon::HttpResponse::
                            newHttpJsonResponse(error);

                        response->setStatusCode(
                            drogon::k500InternalServerError);

                        (*responseCallback)(response);
                    },

                    rollNo,
                    subjectName,
                    marks
                );
            },
            { drogon::Post }
        );

        // --------------------------------------------------
        // UPDATE SUBJECT MARKS
        // PUT /api/students/{roll_no}/marks/{subject_name}
        // JSON: {"marks":95}
        // --------------------------------------------------
        drogon::app().registerHandler(
            "/api/students/{1}/marks/{2}",
            [](const drogon::HttpRequestPtr& req,
                std::function<void(
                    const drogon::HttpResponsePtr&)>&& callback,
                long long rollNo,
                const std::string& subjectName)
            {
                auto responseCallback =
                    std::make_shared<
                    std::function<void(
                        const drogon::HttpResponsePtr&)>
                    >(std::move(callback));

                auto json = req->getJsonObject();

                if (!json ||
                    !json->isMember("marks") ||
                    !(*json)["marks"].isIntegral())
                {
                    Json::Value error;
                    error["error"] =
                        "Integer marks are required";

                    auto response =
                        drogon::HttpResponse::
                        newHttpJsonResponse(error);

                    response->setStatusCode(
                        drogon::k400BadRequest);

                    (*responseCallback)(response);
                    return;
                }

                int newMarks = (*json)["marks"].asInt();

                if (rollNo <= 0 ||
                    subjectName.empty() ||
                    newMarks < 0 ||
                    newMarks > 100)
                {
                    Json::Value error;
                    error["error"] =
                        "Invalid roll number, subject, or marks (0-100)";

                    auto response =
                        drogon::HttpResponse::
                        newHttpJsonResponse(error);

                    response->setStatusCode(
                        drogon::k400BadRequest);

                    (*responseCallback)(response);
                    return;
                }

                auto dbClient =
                    drogon::app().getDbClient("default");

                // Update the requested subject
                dbClient->execSqlAsync(
                    "UPDATE student_marks "
                    "SET marks = ? "
                    "WHERE roll_no = ? AND subject_name = ?",

                    [responseCallback, dbClient, rollNo,
                    subjectName, newMarks](
                        const drogon::orm::Result& result)
                    {
                        // MySQL reports the number of changed rows.
                        // A value of zero can also mean the submitted
                        // marks were already the stored value.
                        dbClient->execSqlAsync(
                            "SELECT COUNT(*) AS subject_count "
                            "FROM student_marks "
                            "WHERE roll_no = ? AND subject_name = ?",

                            [responseCallback, dbClient, rollNo,
                            subjectName, newMarks](
                                const drogon::orm::Result& checkResult)
                            {
                                if (checkResult.empty() ||
                                    checkResult[0]["subject_count"].
                                    as<long long>() == 0)
                                {
                                    Json::Value error;
                                    error["error"] =
                                        "Student subject record not found";

                                    auto response =
                                        drogon::HttpResponse::
                                        newHttpJsonResponse(error);

                                    response->setStatusCode(
                                        drogon::k404NotFound);

                                    (*responseCallback)(response);
                                    return;
                                }

                                // Recalculate average
                                dbClient->execSqlAsync(
                                    "SELECT AVG(marks) AS average "
                                    "FROM student_marks "
                                    "WHERE roll_no = ?",

                                    [responseCallback, dbClient,
                                    rollNo, subjectName, newMarks](
                                        const drogon::orm::Result& avgResult)
                                    {
                                        if (avgResult.empty() ||
                                            avgResult[0]["average"].isNull())
                                        {
                                            Json::Value error;
                                            error["error"] =
                                                "Could not calculate average";

                                            auto response =
                                                drogon::HttpResponse::
                                                newHttpJsonResponse(error);

                                            response->setStatusCode(
                                                drogon::k500InternalServerError);

                                            (*responseCallback)(response);
                                            return;
                                        }

                                        double average =
                                            avgResult[0]["average"].
                                            as<double>();

                                        std::string grade =
                                            calculateGrade(average);

                                        // Save average and grade
                                        dbClient->execSqlAsync(
                                            "UPDATE student "
                                            "SET average = ?, grade = ? "
                                            "WHERE roll_no = ?",

                                            [responseCallback, rollNo,
                                            subjectName, newMarks,
                                            average, grade](
                                                const drogon::orm::Result&)
                                            {
                                                Json::Value result;
                                                result["message"] =
                                                    "Marks updated successfully";
                                                result["roll_no"] =
                                                    Json::Int64(rollNo);
                                                result["subject_name"] =
                                                    subjectName;
                                                result["marks"] = newMarks;
                                                result["average"] = average;
                                                result["grade"] = grade;

                                                auto response =
                                                    drogon::HttpResponse::
                                                    newHttpJsonResponse(result);

                                                (*responseCallback)(response);
                                            },

                                            [responseCallback](
                                                const drogon::orm::
                                                DrogonDbException& e)
                                            {
                                                Json::Value error;
                                                error["error"] =
                                                    e.base().what();

                                                auto response =
                                                    drogon::HttpResponse::
                                                    newHttpJsonResponse(error);

                                                response->setStatusCode(
                                                    drogon::k500InternalServerError);

                                                (*responseCallback)(response);
                                            },

                                            average, grade, rollNo
                                        );
                                    },

                                    [responseCallback](
                                        const drogon::orm::DrogonDbException& e)
                                    {
                                        Json::Value error;
                                        error["error"] = e.base().what();

                                        auto response =
                                            drogon::HttpResponse::
                                            newHttpJsonResponse(error);

                                        response->setStatusCode(
                                            drogon::k500InternalServerError);

                                        (*responseCallback)(response);
                                    },

                                    rollNo
                                );
                            },

                            [responseCallback](
                                const drogon::orm::DrogonDbException& e)
                            {
                                Json::Value error;
                                error["error"] = e.base().what();

                                auto response =
                                    drogon::HttpResponse::
                                    newHttpJsonResponse(error);

                                response->setStatusCode(
                                    drogon::k500InternalServerError);

                                (*responseCallback)(response);
                            },

                            rollNo, subjectName
                        );
                    },

                    [responseCallback](
                        const drogon::orm::DrogonDbException& e)
                    {
                        Json::Value error;
                        error["error"] = e.base().what();

                        auto response =
                            drogon::HttpResponse::
                            newHttpJsonResponse(error);

                        response->setStatusCode(
                            drogon::k500InternalServerError);

                        (*responseCallback)(response);
                    },

                    newMarks, rollNo, subjectName
                );
            },
            { drogon::Put }
        );
        // DELETE a student by roll number
        drogon::app().registerHandler(
            "/api/students/{1}",
            [](const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                long long roll_no)
            {
                auto db = drogon::app().getDbClient();

                db->execSqlAsync(
                    "DELETE FROM student WHERE roll_no = ?",
                    [callback, roll_no](const drogon::orm::Result& result)
                    {
                        Json::Value response;

                        if (result.affectedRows() == 0)
                        {
                            response["message"] = "Student not found";
                            auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
                            resp->setStatusCode(drogon::k404NotFound);
                            callback(resp);
                            return;
                        }

                        response["message"] = "Student deleted successfully";
                        response["roll_no"] = Json::Int64(roll_no);

                        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
                        callback(resp);
                    },
                    [callback](const drogon::orm::DrogonDbException& e)
                    {
                        Json::Value response;
                        response["error"] = "Failed to delete student";

                        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
                        resp->setStatusCode(drogon::k500InternalServerError);
                        callback(resp);
                    },
                    roll_no);
            },
            { drogon::Delete });
        // Enable CORS for the Live Server frontend
        drogon::app().registerPostHandlingAdvice(
            [](const drogon::HttpRequestPtr& req,
                const drogon::HttpResponsePtr& resp)
            {
                resp->addHeader(
                    "Access-Control-Allow-Origin",
                    "http://127.0.0.1:5500"
                );

                resp->addHeader(
                    "Access-Control-Allow-Methods",
                    "GET, POST, PUT, DELETE, OPTIONS"
                );

                resp->addHeader(
                    "Access-Control-Allow-Headers",
                    "Content-Type"
                );
            }
        );

        // Handle CORS preflight requests before route matching.
        drogon::app().registerPreRoutingAdvice(
            [](const drogon::HttpRequestPtr& req,
                drogon::AdviceCallback&& callback,
                drogon::AdviceChainCallback&& chainCallback)
            {
                if (req->method() == drogon::Options)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();

                    resp->addHeader(
                        "Access-Control-Allow-Origin",
                        "http://127.0.0.1:5500"
                    );
                    resp->addHeader(
                        "Access-Control-Allow-Methods",
                        "GET, POST, PUT, DELETE, OPTIONS"
                    );
                    resp->addHeader(
                        "Access-Control-Allow-Headers",
                        "Content-Type"
                    );

                    callback(resp);
                    return;
                }

                chainCallback();
            });

        // Add CORS headers to normal API responses too.
        drogon::app().registerPostHandlingAdvice(
            [](const drogon::HttpRequestPtr& req,
                const drogon::HttpResponsePtr& resp)
            {
                resp->addHeader(
                    "Access-Control-Allow-Origin",
                    "http://127.0.0.1:5500"
                );
            });


        std::cout << "STEP 3: Routes registered" << std::endl;
        std::cout << "STEP 4: Starting server" << std::endl;

        drogon::app().run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
