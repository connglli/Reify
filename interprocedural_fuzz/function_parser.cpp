#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <stdint.h>
#include <cstring>
#include <cassert>
#include <random>
#include <filesystem>
#include "params.hpp"
const int getWithinRange = INT32_MAX;
std::vector<uint32_t> crc32_table(256);
const std::string newProcedureDirectory = "new_procedure/";
std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return ""; // String is all whitespace
    }
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}
void generate_crc32_table(std::vector<uint32_t>& crc32_table) {
    const uint32_t poly = 0xEDB88320UL;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 8; j > 0; j--) {
            if (crc & 1) {
                crc = (crc >> 1) ^ poly;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
}
int32_t uint32_to_int32(uint32_t u) {
    return u%getWithinRange;
}
static uint32_t context_free_crc32_byte(uint32_t context, uint8_t b) {
    return ((context >> 8) & 0x00FFFFFF) ^ crc32_table[(context ^ b) & 0xFF];
}
static uint32_t context_free_crc32_4bytes(uint32_t context, uint32_t val) {
    context = context_free_crc32_byte(context, (val >> 0) & 0xFF);
    context = context_free_crc32_byte(context, (val >> 8) & 0xFF);
    context = context_free_crc32_byte(context, (val >> 16) & 0xFF);
    context = context_free_crc32_byte(context, (val >> 24) & 0xFF);
    return context;
}
std::string generateCodeForChecksumFunction(int checksumType = globalChecksumType)
{
    std::stringstream code;
    //a C-style va-list function to take a variable number of arguments
    switch(checksumType)
    {
        case 0:
            code << "#include <string.h>\n";
            code << "uint32_t crc32_tab[256];\n";
            code << "int32_t uint32_to_int32(uint32_t u){\n";
            code << "    return u%"<< getWithinRange << ";\n";
            code << "}\n";
            code << "void generate_crc32_table(uint32_t* crc32_table) {\n";
            code << "    const uint32_t poly = 0xEDB88320UL;\n";
            code << "    for (uint32_t i = 0; i < 256; i++) {\n";
            code << "        uint32_t crc = i;\n";
            code << "        for (int j = 8; j > 0; j--) {\n";
            code << "            if (crc & 1) {\n";
            code << "                crc = (crc >> 1) ^ poly;\n";
            code << "            } else {\n";
            code << "                crc >>= 1;\n";
            code << "            }\n";
            code << "        }\n";
            code << "        crc32_table[i] = crc;\n";
            code << "    }\n";
            code << "}\n";
            code << "uint32_t context_free_crc32_byte(uint32_t context, uint8_t b) {\n";
            code << "    return ((context >> 8) & 0x00FFFFFF) ^ crc32_tab[(context ^ b) & 0xFF];\n";
            code << "}\n";
            code << "uint32_t context_free_crc32_4bytes(uint32_t context, uint32_t val) {\n";
            code << "    context = context_free_crc32_byte(context, (val >> 0) & 0xFF);\n";
            code << "    context = context_free_crc32_byte(context, (val >> 8) & 0xFF);\n";
            code << "    context = context_free_crc32_byte(context, (val >> 16) & 0xFF);\n";
            code << "    context = context_free_crc32_byte(context, (val >> 24) & 0xFF);\n";
            code << "    return context;\n";
            code << "}\n";
        default:
            assert(false && "Invalid checksum type");
    }
    code << "int computeStatelessChecksum(int num_args, ...)\n";
    code << "{\n";
    code << "    va_list args;\n";
    code << "    va_start(args, num_args);\n";
    switch(checksumType)
    {
        case 0:
            code << "    uint32_t checksum = 0xFFFFFFFFUL;\n";
            code << "    for (int i = 0; i < num_args; ++i) {\n";
            code << "        int arg = va_arg(args, int);\n";
            code << "        checksum = context_free_crc32_4bytes(checksum, arg);\n";
            code << "    }\n";
            code << "    checksum = uint32_to_int32(checksum ^ 0xFFFFFFFFUL);\n";
            break;
        case 1:
            code << "    int mod = 1000000007;\n";
            code << "    long checksum = 0;\n";
            code << "    for (int i = 0; i < num_args; ++i) {\n";
            code << "        int arg = va_arg(args, int);\n";
            code << "        checksum = (checksum + (long)arg) % (long)mod;\n";
            code << "        checksum = (checksum + (long)mod) % (long)mod;\n";
            code << "    }\n";
            code << "    checksum = (checksum + (long)mod) % (long)mod;\n";
            break;
        default:
            assert(false && "Invalid checksum type");
    }
    code << "    va_end(args);\n";
    code << "    return checksum;\n";
    code << "}\n";
    return code.str();

}
static uint32_t computeStatelessChecksum(std::vector<int> initialisation, int checksumType = globalChecksumType)
{
    switch(checksumType)
    {
        case 0:{
            uint32_t checksum = 0xFFFFFFFFUL;
            for (size_t i = 0; i < initialisation.size(); ++i) {
                checksum = context_free_crc32_4bytes(checksum, initialisation[i]);
            }
            return uint32_to_int32(checksum ^ 0xFFFFFFFFUL);
        }
        case 1:
        {
            long checksum = 0;
            for(auto i : initialisation)
            {
                checksum = (checksum + (long)i)%(long)mod;
                checksum = (checksum + (long)mod)%(long)mod;
            }
            checksum = (checksum + (long)mod)%(long)mod;
            return checksum;
        }
        default:
        {
            assert(false && "Invalid checksum type");
            return 0;
        }
    }
    
}
class Statement
{
    public:
    enum class Type
    {
        ASSIGNMENT,
        IF,
        FLUFF
    };
    Type type;
    virtual std::string generateCode() const = 0;
    virtual Type getType() const = 0;
    virtual bool replaceCoefficient(std::string procedureName, std::vector<int> initialisation, std::vector<int> final_value) = 0;
    virtual int getNumCoefficients() const { return 0; };
    // Statement() = default;
};
class FluffStatement : public Statement
{
    std::string code;
    public:
        FluffStatement(std::string code) : code(code) {}
        std::string generateCode() const override
        {
            return code;
        }
        Type getType() const override
        {
            return Type::FLUFF;
        }
        bool replaceCoefficient(std::string procedureName, std::vector<int> initialisation, std::vector<int> final_value)
        {
            return false;
        }
};
using Coefficient = std::string;
using Variable = std::string;
using isCoefficientMutated = bool;
class AssignmentStatement : public Statement
{
    std::string variableLHS;
    std::vector<std::pair<std::pair<Coefficient, isCoefficientMutated>, Variable>> RHSExpression; //coefficient_value, is_mutated, variable
    public:
        AssignmentStatement(std::string statement)
        {
            size_t equalPos = statement.find('=');
            variableLHS = statement.substr(0, equalPos);
            std::string rhs = statement.substr(equalPos + 1);
            std::stringstream ss(rhs);
            std::string token;
            while (std::getline(ss, token, '+') || std::getline(ss, token, ';')) {
                size_t mulPos = token.find('*');
                if (mulPos != std::string::npos) {
                    Coefficient coeff = token.substr(0, mulPos);
                    Variable var = token.substr(mulPos + 1);
                    bool is_mutated = false; // Placeholder for mutation check
                    RHSExpression.push_back({{coeff, is_mutated}, var});
                }
                else {
                    mulPos = token.find(';');
                    if (mulPos != std::string::npos) {
                        token = token.substr(0, mulPos);
                    }
                    Coefficient coeff = token;
                    Variable var = "1";
                    bool is_mutated = false; // Placeholder for mutation check
                    RHSExpression.push_back({{token, is_mutated}, "1"}); // Default coefficient of 1
                }
            }
        }
        std::string generateCode() const override
        {
            std::string code = variableLHS + " = ";
            for (size_t i = 0; i < RHSExpression.size(); ++i) {
                code += "("+RHSExpression[i].first.first + ") * " + RHSExpression[i].second;
                if (i != RHSExpression.size() - 1) {
                    code += " + ";
                }
            }
            code += ";";
            return code;
        }
        Type getType() const override
        {
            return Type::ASSIGNMENT;
        }
        int getNumCoefficients() const override
        {
            return RHSExpression.size();
        }
        bool replaceCoefficient(std::string procedureName, std::vector<int> initialisation, std::vector<int> final_value)
        {
            for (auto& term : RHSExpression)
            {
                if(! term.first.second)
                {
                    //replace the coefficient with a call to the procedure
                    int coefficient = std::stoi(term.first.first);
                    int checksum = computeStatelessChecksum(final_value);
                    std::string replacement = procedureName + "(";
                    for(int i = 0; i < initialisation.size() - 1; ++i)
                    {
                        replacement += std::to_string(initialisation[i]) + ", ";
                    }
                    replacement += std::to_string(initialisation[initialisation.size() - 1]);
                    replacement += ") + " + std::to_string(coefficient - checksum);
                    if((long)coefficient - (long)checksum >= (long)INT32_MIN && (long)coefficient - (long)checksum <= (long)INT32_MAX)
                    {
                        term.first.first = replacement;
                        term.first.second = true;
                        return true;
                    }
                    else
                    {
                        
                        std::string replacement = procedureName + "(";
                        for(int i = 0; i < initialisation.size() - 1; ++i)
                        {
                            replacement += std::to_string(initialisation[i]) + ", ";
                        }
                        replacement += std::to_string(initialisation[initialisation.size() - 1]);    
                        replacement += ")";
                        term.first.first = "(long)" + replacement + " + (long)" + std::to_string(coefficient) + " - (long) " + std::to_string(checksum);
                        term.first.second = true;
                        return true;
                    }
                }
            }
            return false;
        }
};

class IfStatement: public Statement
{
    std::vector<std::pair<std::pair<Coefficient, isCoefficientMutated>, Variable>> condition;
    std::string gotoStatement;
    public:
        IfStatement(std::string statement)
        {
            size_t ifPos = statement.find("if");
            size_t openParen = statement.find('(', ifPos);
            size_t closeParen = statement.find(')', openParen);
            std::string condition_str = statement.substr(openParen + 1, closeParen - openParen - 1);
            std::stringstream ss(condition_str);
            std::string token;
            while (std::getline(ss, token, '+') || std::getline(ss, token, ';')) {
                size_t mulPos = token.find('*');
                if (mulPos != std::string::npos) {
                    Coefficient coeff = token.substr(0, mulPos);
                    Variable var = token.substr(mulPos + 1);
                    bool is_mutated = false; // Placeholder for mutation check
                    condition.push_back({{coeff, is_mutated}, var});
                }
                else {
                    mulPos = token.find(">=");
                    if (mulPos != std::string::npos) {
                        token = token.substr(0, mulPos);
                    }
                    Coefficient coeff = token;
                    Variable var = "1";
                    bool is_mutated = false; // Placeholder for mutation check
                    condition.push_back({{token, is_mutated}, "1"}); // Default coefficient of 1
                }
            }
            gotoStatement = statement.substr(closeParen + 1);
        }
        int getNumCoefficients() const
        {
            return condition.size();
        }
        // 
        bool replaceCoefficient(std::string procedureName, std::vector<int> initialisation, std::vector<int> final_value)
        {
            for (auto& term : condition)
            {
                if(! term.first.second)
                {
                    //replace the coefficient with a call to the procedure
                    int coefficient = std::stoi(term.first.first);
                    int checksum = computeStatelessChecksum(final_value);
                    std::string replacement = procedureName + "(";
                    for(int i = 0; i < initialisation.size() - 1; ++i)
                    {
                        replacement += std::to_string(initialisation[i]) + ", ";
                    }
                    replacement += std::to_string(initialisation[initialisation.size() - 1]);
                    replacement += ") + " + std::to_string(coefficient - checksum);
                    if((long)coefficient - (long)checksum >= (long)INT32_MIN && (long)coefficient - (long)checksum <= (long)INT32_MAX)
                    {
                        term.first.first = replacement;
                        term.first.second = true;
                        return true;
                    }
                    else
                    {
                        
                        std::string replacement = procedureName + "(";
                        for(int i = 0; i < initialisation.size() - 1; ++i)
                        {
                            replacement += std::to_string(initialisation[i]) + ", ";
                        }
                        replacement += std::to_string(initialisation[initialisation.size() - 1]);
    
                        replacement += ")";
                        term.first.first = "(long)" + replacement + " + (long)" + std::to_string(coefficient) + " - (long) "+ std::to_string(checksum);
                        term.first.second = true;
                        return true;
                    }
                }
            }
            return false;
        }
        std::string generateCode() const override
        {
            std::string code = "if (";
            for (size_t i = 0; i < condition.size(); ++i) {
                code += "(" + condition[i].first.first + ") * " + condition[i].second;
                if (i != condition.size() - 1) {
                    code += " + ";
                }
            }
            code += " >= 0) " + gotoStatement;
            return code;
        }   
        Type getType() const override
        {
            return Type::IF;
        }     
};
class Procedure{
    public:
    std::string name;
    int num_input_vars;
    int total_num_coefficients = 0;
    std::unordered_map<std::string, int> parameters;
    std::vector<std::unique_ptr<Statement>> statements;
    std::vector<std::vector<int>> initialisations;
    std::vector<std::vector<int>> final_values;
    Procedure() {}
    Procedure(std::ifstream &procedureFileName, std::ifstream& mappingFileName)
    {
        std::string line;
        while (std::getline(procedureFileName, line)) {
            if(line.find("if") != std::string::npos)
            {
                statements.push_back(std::make_unique<IfStatement>(line));
                total_num_coefficients += statements.back()->getNumCoefficients();
            }
            else if (line.find("int") != std::string::npos) {
                if(line.find("pass_counter") != std::string::npos)
                {
                    statements.push_back(std::make_unique<FluffStatement>(line));
                    continue;
                }
                name = line.substr(line.find("int") + 4, line.find('(') - line.find("int") - 4);
                size_t start = line.find('(') + 1;
                size_t end = line.find(')');
                std::string params = line.substr(start, end - start);
                num_input_vars = std::count(params.begin(), params.end(), ',') + 1;
                statements.push_back(std::make_unique<FluffStatement>(line));
            }
            else if(line.find("return") != std::string::npos) {
                // End of the procedure
                //modify the return computeStatelessChecksum function to take in num_input_vars as an argument
                std::string newLine = "return computeStatelessChecksum(" + std::to_string(num_input_vars);
                for(int i = 0; i < num_input_vars; ++i)
                {
                    newLine += ", var_" + std::to_string(i);
                }
                newLine += ");";
                statements.push_back(std::make_unique<FluffStatement>(newLine));
                // statements.push_back(std::make_unique<FluffStatement>(line));
            }
             else if (line.find("BB") != std::string::npos) {
                // Start a new basic block
                statements.push_back(std::make_unique<FluffStatement>(line));

            } else if (line.find("=") != std::string::npos) {
                // Add statement to the current basic block
                statements.push_back(std::make_unique<AssignmentStatement>(line));
                total_num_coefficients += statements.back()->getNumCoefficients();
            }
            else
            {
                // Add fluff statement
                statements.push_back(std::make_unique<FluffStatement>(line));
            }
        }
        //sample mappings file
        // 1,1,1,-429582,-2147483647, : -40705420,2116681841,2122492187,-989376752,-2147483647,
        // 1,1,1,-429582,-2147483647, : -40705420,2116681841,2122492187,-989376752,-2147483647,
        // 1,1,1,-429582,-2147483647, : -40705420,2116681841,2122492187,-989376752,-2147483647,
        // std::cout<< "Reading mapping file: " << std::endl;
        while(std::getline(mappingFileName, line))
        {
            // std::cout << "lINE IS"<<line << std::endl;
            std::stringstream ss(line);
            std::string token;
            std::vector<int> mapping;
            //split line at :, and store the first part in initialisations and the second part in final_values
            std::getline(ss, token, ':');
            std::stringstream ss1(token);
            while (std::getline(ss1, token, ',')) {
                if (!token.empty()) {
                    // std::cout << token << std::endl;
                    try{
                    mapping.push_back(std::stoi(token));
                    }
                    catch(...){
                        continue;
                    }
                }
            }
            initialisations.push_back(mapping);
            mapping.clear();
            //now read the rest of the line
            std::getline(ss, token, ':');
            std::stringstream ss2(token);
            while (std::getline(ss2, token, ',')) {
                if (!token.empty()) {
                    // std::cout << token << std::endl;
                    try{
                    mapping.push_back(std::stoi(token));
                    }
                    catch(...){
                        continue;
                    }
                }
            }
            // std::cout << "mapping founddd" << std::endl;
            final_values.push_back(mapping);
        }
        // std::cout << "Read mapping file successfully" << std::endl;
    }
    std::string getName() const
    {
        return name;
    }
    bool replaceCoefficient(std::string procedureName, std::vector<int> initialisation, std::vector<int> final_value)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0, 1);
        auto probability  = dis(gen);
        int trials = 1000;
        //sample a statement from the list of statements until you get an IF statement
        while(true && trials > 0)
        {
            int index = rand() % statements.size();
            //Idea: 80% of the replacements should be in the IF statements, and 20% in the assignment statements
            if(statements[index]->getType() == Statement::Type::IF && probability < 0.8)
            {
                if(statements[index]->replaceCoefficient(procedureName, initialisation, final_value))
                {
                    return true;
                }
            }
            else if(statements[index]->getType() == Statement::Type::ASSIGNMENT && probability > 0.8) 
            {
                if(statements[index]->replaceCoefficient(procedureName, initialisation, final_value))
                {
                    return true;
                }
            }
            trials--;
        }
        return false;
    }
    std::string generateCode() const
    {
        std::string code;
        for (const auto& statement : statements) {
            code += statement->generateCode() + "\n";
        }
        return code;
    }
    void getMapping(std::vector<int> &foreign_initialisation, std::vector<int> &foreign_final_value) const
    {
        //sample a random index from initialisations list
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, initialisations.size() - 1);
        int index = dis(gen);
        foreign_initialisation = initialisations[index];
        foreign_final_value = final_values[index];    
    }  
    int calculateNumCoefficientsToReplace() const
    {
        return total_num_coefficients*replacementProbability;
    }
};
std::string mappingForProcedure(std::string procedureName)
{
    //remove .c from the procedure name
    size_t pos = procedureName.find(".c");
    if (pos != std::string::npos) {
        procedureName = procedureName.substr(0, pos);
    }
    return procedureName + "_mapping";
}
void generateCodeForInterproceduralBlock(std::string filename, const std::vector<Procedure> &procedures, bool staticModifier = false, int checksumType = globalChecksumType)
{
    std::ofstream outFile(filename);
    outFile << "#include <stdint.h>\n";
    outFile << "#include <stdio.h>\n";
    outFile << "#include <stdarg.h>\n";
    outFile << generateCodeForChecksumFunction(checksumType);
    outFile << "\n";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 1);
    for (int i = PROCEDURE_DEPTH - 1; i >= 0; --i) {
        // output inline with 60% probability
        auto probability = dis(gen);
        if(staticModifier)
        {
            outFile << "static ";
        }
        if (probability < 0.6) {
            if(!staticModifier)
            {
                outFile << "static inline ";
            }
            else
            {
                outFile << "inline ";
            }
        }
        outFile << procedures[i].generateCode() << "\n";
    }
    outFile << "int main() {\n";
    if(checksumType == 0)
    {
        outFile << "    generate_crc32_table(crc32_tab);\n";
    }
        std::vector<int> initialisation;
        std::vector<int> final_value;
        procedures[0].getMapping(initialisation, final_value);
        outFile << "printf(\"%d,\", " << procedures[0].getName() << "(";
        for (int i = 0; i < initialisation.size(); ++i) {
            outFile << initialisation[i];
            if (i != initialisation.size() - 1) {
                outFile << ", ";
            }
        }
        outFile << "));\n";
    outFile << "return 0;\n";
    outFile << "}\n";
    outFile.close();
    // std::cout << "Code generated successfully in " << filename << std::endl;

}
int main(int argc, char* argv[])
{
    if(argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <procedure_directory> <mapping_directory> <uuid>" << std::endl;
        return 1;
    }
    std::string str1(argv[1]);
    std::string str2(argv[2]);
    std::string procedureDirectory = str1;
    std::string mappingDirectory = str2;
    std::string new_procedure_uuid = argv[3];
    std::string newProcedureDirectory = "new_procedures";
    //read all files from procedureDirectory
    std::ifstream procedureFileName(procedureDirectory);
    std::vector<std::string> procedureFiles;
    //open the directory and read all files
    for (const auto& entry : std::filesystem::directory_iterator(procedureDirectory)) {
        if (entry.is_regular_file()) {
            procedureFiles.push_back(entry.path().string());
        }
    }
    generate_crc32_table(crc32_table);
    std::vector<std::string> selectedFiles;
    selectedFiles.resize(PROCEDURE_DEPTH);
    std::vector<Procedure> procedures;
    procedures.resize(PROCEDURE_DEPTH);    
    int sample_ct = 0;
    while(true)
    {
        sample_ct++;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, procedureFiles.size() - 1); 
        //choose the procedure files randomly
        memset(selectedFiles.data(), 0, selectedFiles.size() * sizeof(std::string));
        for (int i = 0; i < PROCEDURE_DEPTH; ++i) {
            while(true)
            {
                int index = dis(gen);
                if(std::find(selectedFiles.begin(), selectedFiles.end(), procedureFiles[index]) == selectedFiles.end())
                {
                    selectedFiles[i] = (procedureFiles[index]);
                    break;
                }
            }
        }
        for (int i = 0; i < PROCEDURE_DEPTH; ++i) {
            std::string file = selectedFiles[i];

            file = file.substr(file.find_last_of("/\\") + 1);
            std::ifstream procedureFile(procedureDirectory + "/" +file);
            std::ifstream mappingFile(mappingDirectory + "/" + mappingForProcedure(file));
            std::cout<<"Opening file: " << procedureDirectory + "/" +file << std::endl;
            std::cout<<"Opening file: " << mappingDirectory + "/" + mappingForProcedure(file) << std::endl;
            if (!procedureFile.is_open() || !mappingFile.is_open()) {
                std::cerr << "Error opening file: " << file << std::endl;
                continue;
            }
            procedures[i] = Procedure(procedureFile, mappingFile);
            // std::cout << "Procedure name: " << procedures[i].getName() << std::endl;
            procedureFile.close();
            mappingFile.close();
        }
        std::vector<int> initialisation;
        std::vector<int> final_value;
        //now replace the mappings in the procedures with the calls to the other procedures
        for(int i = 0; i < PROCEDURE_DEPTH; ++i) {
            int num_mappings_to_replace = procedures[i].calculateNumCoefficientsToReplace();
            // std::cout << "Number of mappings to replace: " << num_mappings_to_replace << std::endl;
            //sample a procedure from i + 1 to PROCEDURE_DEPTH
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(i + 1, PROCEDURE_DEPTH - 1);
            if (i == PROCEDURE_DEPTH - 1) {
                break;
            }
            for(int k = 0; k < num_mappings_to_replace; ++k)
            {
                int index = dis(gen);
                procedures[index].getMapping(initialisation, final_value);
                if(! procedures[i].replaceCoefficient(procedures[index].getName(), initialisation, final_value)) break;
                // std::cout<<"Replacing mapping for procedure: " << procedures[index].getName() << " in procedure: " << procedures[i].getName() << std::endl;
            }

        }
        // std::cout<< "Now generating code for interprocedural block" << std::endl;
        generateCodeForInterproceduralBlock(newProcedureDirectory + "/" + new_procedure_uuid + "_" + std::to_string(sample_ct) + ".c", procedures);
        generateCodeForInterproceduralBlock(newProcedureDirectory + "/" + new_procedure_uuid + "_" + std::to_string(sample_ct) + "_static.c", procedures, true);
    }

}