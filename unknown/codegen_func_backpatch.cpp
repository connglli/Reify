#include <bits/stdc++.h>
#include "better_graph.hpp"
#include"z3++.h"
#include <optional>
#include "params.hpp"
#include <signal.h>
#include <unistd.h>
std::ofstream logFile;
static bool isFormulaSatisfiable = false;
using VarIndex = int;
using BasicBlockNo = int;

z3::expr atMostKZeroes(z3::context& ctx, const std::vector<z3::expr>& vec, int k, int val) {
    z3::expr_vector zero_constraints(ctx);
    
    for (const auto& expr : vec) {
        zero_constraints.push_back(z3::ite(expr == val, ctx.int_val(1), ctx.int_val(0)));
    }
    
    z3::expr sum_zeros = sum(zero_constraints);
    return sum_zeros <= k;
}
std::unordered_map<std::string, std::optional<int>> parameters;
std::vector<int> sampleKDistinct(int n, int k) {
    n -= 1;
    if (k > n + 1) {
        throw std::invalid_argument("Error: k must be at most n + 1 to sample k distinct numbers.");
    }    
    std::vector<int> numbers(n + 1);
    for (int i = 0; i <= n; ++i) {
        numbers[i] = i;
    }
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(numbers.begin(), numbers.end(), gen);
    
    return std::vector<int>(numbers.begin(), numbers.begin() + k);
}
//MOTIVATION: We need to generate a pass counter name for the block in case we end up in a consistent walk with a loop which never reaches the end node
std::string generatePassCounterName(int blockno)
{
    return "pass_counter_" + std::to_string(blockno);
}
std::string generateWalkTypeVariable()
{
    return "walk_type";
}
std::string getVarName(int varIndex)
{
    return "var_" + std::to_string(varIndex);
}
std::string getVarNameForWalk(int walk_init_index, int varIndex)
{
    return "var_" + std::to_string(varIndex) + "_walk_" + std::to_string(walk_init_index);
}
std::string getCoefficientName(int blockno, int statementIndex, int statementSubIndex)
{
    return "a_" + std::to_string(blockno) + "_" + std::to_string(statementIndex) +  "_" + std::to_string(statementSubIndex);
}
std::string generateConstantName(int blockno, int statementIndex)
{
    return "a_" + std::to_string(blockno) + "_" + std::to_string(statementIndex);
}
std::string generateLabelName(int blockno)
{
    return "BB" + std::to_string(blockno);
}
std::string generateConditionalCoefficientName(int blockno, int statementIndex, int statementSubIndex)
{
    return "b_" + std::to_string(blockno) + "_" + std::to_string(statementIndex) +  "_" + std::to_string(statementSubIndex);
}
std::string generateConditionalConstantName(int blockno, int statementIndex)
{
    return "b_" + std::to_string(blockno) + "_" + std::to_string(statementIndex);
}
std::string generateConditionalConstraint(int blockno, int target, std::vector<VarIndex> conditionalVariables)
{
    std::ostringstream constraint;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(LOWER_BOUND, UPPER_BOUND);
    constraint << "    if (";
    int ctr = 0;
    for (auto i : conditionalVariables)
    {
        std::string coefficientKey = generateConditionalCoefficientName(blockno, 0, ctr);
        int coefficient;
        if (parameters.find(coefficientKey) != parameters.end() && parameters[coefficientKey].has_value())
        {
            coefficient = parameters[coefficientKey].value();
        }
        else
        {
            coefficient = dist(gen);
            parameters[coefficientKey] = coefficient;
        }
        constraint << coefficient << " * " << getVarName(i);
        constraint << " + ";
        ++ctr;
    }
    std::string constantKey = generateConditionalConstantName(blockno, target);
    int constant;
    if (parameters.find(constantKey) != parameters.end() && parameters[constantKey].has_value())
    {
        constant = parameters[constantKey].value();
    }
    else
    {
        constant = dist(gen);
        parameters[constantKey] = constant;
    }
    constraint  << constant;
    constraint << " >= 0) goto " << generateLabelName(target) << ";\n";

    // Output the constraint
    return constraint.str();
}
std::string generateUnconditionalGoto(int target)
{
    std::ostringstream code;
    code << "    goto " << generateLabelName(target) << ";\n";
    return code.str();
}
z3::expr makeCoefficientsInteresting(const std::vector<z3::expr>& coeffs, z3::context& c)
{
    // z3::expr allZero = c.bool_const(("true"));
    // for(auto coeff: coeffs)
    // {
    //     allZero = allZero && (coeff == 0);
    // }
    // return !allZero;
    if(!enableInterestingCoefficients)
    {
        return c.bool_val(true);
    }
    return atMostKZeroes(c, coeffs, coeffs.size()/2, 0);
}
z3::expr addRandomInitialisations(int walk_init_index, z3::context& c)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(LOWER_INIT_BOUND, UPPER_INIT_BOUND);
    std::vector<z3::expr> allVars;
    z3::expr allAssignedConstraint = c.bool_val(true);
    for (int i = 0; i < NUM_VARS; i++) 
    {
        allVars.push_back(c.int_const((getVarNameForWalk(walk_init_index, i)+ "_0").c_str()));
        z3::expr randomInit = c.bool_val(true);
        if (parameters.find(getVarNameForWalk(walk_init_index, i)) != parameters.end() && parameters[getVarNameForWalk(walk_init_index, i)].has_value())
        {
            randomInit = (allVars[i] == parameters[getVarNameForWalk(walk_init_index, i)].value());
        }
        else
        {
            int randomValue = dist(gen);
            parameters[getVarNameForWalk(walk_init_index, i)] = randomValue;
            randomInit = (allVars[i] == randomValue);
        }
        allAssignedConstraint = allAssignedConstraint && randomInit;
    }
    //let's assign each variable a random value between -100 and 10
    return allAssignedConstraint;
}
z3::expr boundCoefficients(z3::context& c, const std::vector<z3::expr>& coeffs)
{
    z3::expr allAssignedConstraint = c.bool_val(true);
    for (const auto& coeff : coeffs) 
    {
        allAssignedConstraint = allAssignedConstraint && (coeff >= LOWER_COEFF_BOUND && coeff <= UPPER_COEFF_BOUND);
    }
    return allAssignedConstraint;
}
z3::expr makeInitialisationsInteresting(z3::context& c, int walk_init_index)
{
    if(!enableInterestingInitialisations)
    {
        return c.bool_val(true);
    }
    std::vector<z3::expr> allVars;
    for (int i = 0; i < NUM_VARS; i++) 
    {
        allVars.push_back(c.int_const((getVarNameForWalk(walk_init_index, i)+ "_0").c_str()));
    }
    //let's assign each variable a random value between -100 and 10
    return atMostKZeroes(c, allVars, NUM_VARS/2, 0);
}
class BB{
    public:
    BasicBlockNo blockno;
    int numStatements;
    std::map<VarIndex, std::vector<VarIndex>> statementMappings;
    std::vector<VarIndex> assignmentOrder;
    std::vector<BasicBlockNo> blockTargets;
    std::vector<std::vector<VarIndex>> conditionalVariablesForAllBranches;
    bool needPassCounter = false;
    std::vector<std::pair<int, int>> passCounterValues;
    public:
        BB(BasicBlockNo blockno, const std::set<BasicBlockNo>& graphTargets) : blockno(blockno), needPassCounter(false)
        {
            numStatements = ASSIGNMENTS_PER_BB;
            assignmentOrder = sampleKDistinct(NUM_VARS, ASSIGNMENTS_PER_BB);
            int statementIndex = 0;
            for(auto varIndex : assignmentOrder)
            {
                statementMappings[varIndex] = sampleKDistinct(NUM_VARS, VARIABLES_PER_ASSIGNMENT_STATEMENT);

                //check every element present in vector is less than NUM_VARS
                for(auto var: statementMappings[varIndex])
                {
                    if(var >= NUM_VARS)
                    {
                        throw std::invalid_argument("Error: variable index out of bounds");
                    }
                }
                // if(std::find(statementMappings[varIndex].begin(), statementMappings[varIndex].end(), varIndex) == statementMappings[varIndex].end())
                // {
                //     statementMappings[varIndex].push_back(varIndex);
                // }
                for(int i = 0; i < statementMappings[varIndex].size(); i++)
                {
                    std::string coefficientKey =    getCoefficientName(blockno, statementIndex, i);
                    parameters[coefficientKey] = std::nullopt;
                }
                std::string constantKey = generateConstantName(blockno, statementIndex);
                parameters[constantKey] = std::nullopt;
                statementIndex++;
                //sample a key from the vector assignmentOrder and add it to conditionalVariables
            }
            for(int i = 0; i < graphTargets.size() - 1; i++)
            {
                std::vector<VarIndex> conditionalVariables;
                conditionalVariables = assignmentOrder;
                logFile << "size of conditional variables: "<<conditionalVariables.size()<<'\n';
                logFile << NUM_VARIABLES_IN_CONDITIONAL<<'\n';
                //now sample another random variable and add it to the conditionalVariables
                for(int i = 0; i < NUM_VARIABLES_IN_CONDITIONAL - assignmentOrder.size(); i++)
                {
                    int randomVariable = std::rand() % NUM_VARS;
                    while(std::find(conditionalVariables.begin(), conditionalVariables.end(), randomVariable) != conditionalVariables.end())
                    {
                        randomVariable = std::rand() % NUM_VARS;
                    }
                    conditionalVariables.push_back(randomVariable);
                }
                conditionalVariablesForAllBranches.push_back(conditionalVariables);
            }
            std::vector<BasicBlockNo> targets;
            for(auto target: graphTargets)
            {
                blockTargets.push_back(target);
            }
            std::random_device rd;
            std::mt19937 gen(rd());
            std::shuffle(blockTargets.begin(), blockTargets.end(), gen);
            for(int j = 0; j < blockTargets.size() - 1; j++)
            {
                std::vector<VarIndex> conditionalVariables = conditionalVariablesForAllBranches[j];
                for(int i = 0; i < conditionalVariables.size(); i++)
                {
                    std::string coefficientKey = generateConditionalCoefficientName(blockno, j, i);
                    parameters[coefficientKey] = std::nullopt;
                }
                std::string constantKey = generateConditionalConstantName(blockno, blockTargets[j]);
                parameters[constantKey] = std::nullopt;
            }
        }
        void setPassCounter(int value, int walkno)
        {
            needPassCounter = true;
            passCounterValues.push_back(std::make_pair(value, walkno));
        }
        std::string generateCode() const
        {
            std::ostringstream code;

            // Generate the label for the basic block
            code << generateLabelName(blockno) << ":\n";
            if(needPassCounter)
            {
                code << generatePassCounterName(blockno) << "++;\n";
            }
            // code<< "    printf(\"starting off at "<<generateLabelName(blockno)<<"\\n\");\n";
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> dist(LOWER_BOUND, UPPER_BOUND);
            int statementIndex = 0;
            for (const auto& [varIndex, dependencies] : statementMappings)
            {
                code << "    " << getVarName(varIndex) << " = ";
                for (size_t i = 0; i < dependencies.size(); ++i)
                {
                    // Look up the coefficient in the parameters map
                    std::string coefficientKey = getCoefficientName(blockno, statementIndex, i);
                    int coefficient;
                    if (parameters.find(coefficientKey) != parameters.end() && parameters[coefficientKey].has_value())
                    {
                        coefficient = parameters[coefficientKey].value();
                    }
                    else
                    {
                        coefficient = dist(gen); 
                        parameters[coefficientKey] = coefficient; // Store it in parameters
                    }

                    // Multiply the dependency by the coefficient
                    code << coefficient << " * " << getVarName(dependencies[i]);
                    if (i < dependencies.size() - 1)
                    {
                        code << " + "; // Example operation
                    }
                }

                // Add a constant term
                std::string constantKey = generateConstantName(blockno, statementIndex);
                int constant;
                if (parameters.find(constantKey) != parameters.end() && parameters[constantKey].has_value())
                {
                    constant = parameters[constantKey].value();
                }
                else
                {
                    constant = dist(gen); 
                    parameters[constantKey] = constant; 
                }
                code << " + " << constant << ";\n";

                ++statementIndex;
            }
            if(needPassCounter)
            {
                for(auto passCounterValue: passCounterValues)
                {
                    code << "    if(" << generatePassCounterName(blockno) << " == " << passCounterValue.first << " && " << generateWalkTypeVariable() << " == " << passCounterValue.second << ") goto " << generateLabelName(NODES - 1) << ";\n";
                }
            }
            if(blockTargets.size() > 1)
            {
                code << generateConditionalConstraint(blockno, blockTargets[0], conditionalVariables);
                code << generateUnconditionalGoto(blockTargets[1]);
            }
            else if(blockTargets.size() == 1)
            {
                code << generateUnconditionalGoto(blockTargets[0]);
            }
            else 
            {
                code << "    computeChecksum(";
                for (int i = 0; i < NUM_VARS; ++i) {
                    code << getVarName(i);
                    if (i < NUM_VARS - 1) {
                        code << ",";
                    }
                }
                code << ");\n";
                
            }
            return code.str();
        }
        z3::expr generateConstraints(int target,  z3::solver& solver, z3::context& c, std::unordered_map<std::string, int>& versions, int walk_init_index)
        {
            //check if all subexpressions are free of Undefined Behaviour
            z3::expr finalGeneratedConstraint = c.bool_val(true);
            int statementIndex = 0;
            for (const auto& [varIndex, dependencies] : statementMappings)
            {
                std::vector<z3::expr> terms;
                // Define integer variables and coefficients
                std::vector<z3::expr> vars;
                std::vector<z3::expr> coeffs;
                for (int i = 0; i < dependencies.size(); i++)
                {
                    vars.push_back(c.int_const((getVarNameForWalk(walk_init_index, dependencies[i]) + "_" + std::to_string(versions[getVarNameForWalk(walk_init_index,dependencies[i])])).c_str()));
                    // solver.add(vars.back() >= LOWER_BOUND && vars.back() <= UPPER_BOUND);
                    finalGeneratedConstraint = finalGeneratedConstraint && (vars.back() >= LOWER_BOUND && vars.back() <= UPPER_BOUND);
                    coeffs.push_back(c.int_const((getCoefficientName(blockno, statementIndex, i)).c_str()));
                    parameters[getCoefficientName(blockno, statementIndex, i)] = std::nullopt;
                    terms.push_back(coeffs.back() * vars.back()); // a_i * var_i
                }
                // solver.add(boundCoefficients(c, coeffs));
                finalGeneratedConstraint = finalGeneratedConstraint && boundCoefficients(c, coeffs);
                terms.push_back(c.int_const((generateConstantName(blockno, statementIndex)).c_str()));
                parameters[generateConstantName(blockno, statementIndex)] = std::nullopt;
                coeffs.push_back(c.int_const((generateConstantName(blockno, statementIndex)).c_str()));
                // solver.add(coeffs.back() >= LOWER_BOUND && coeffs.back() <= UPPER_BOUND);
                finalGeneratedConstraint = finalGeneratedConstraint && (coeffs.back() >= LOWER_BOUND && coeffs.back() <= UPPER_BOUND);
                z3::expr sum = terms[0];
                z3::expr term_constraint = (terms[0] <= UPPER_BOUND) && (terms[0] >= LOWER_BOUND);
                if(enableSafetyChecks) solver.add(term_constraint);
                for (int i = 1; i < terms.size(); i++) {
                    z3::expr constraint = (sum <= UPPER_BOUND) && (sum >= LOWER_BOUND);
                    if(enableSafetyChecks) {
                        // solver.add(constraint);
                        finalGeneratedConstraint = finalGeneratedConstraint && constraint;
                    }
                    sum = sum + terms[i];
                    term_constraint = (terms[i] <= UPPER_BOUND) && (terms[i] >= LOWER_BOUND);
                    if(enableSafetyChecks) {
                        // solver.add(term_constraint);
                        finalGeneratedConstraint = finalGeneratedConstraint && term_constraint;
                    }
                }
                z3::expr constraint = (sum <= UPPER_BOUND) && (sum >= LOWER_BOUND);
                if(enableSafetyChecks) 
                {
                    // solver.add(constraint);
                    finalGeneratedConstraint = finalGeneratedConstraint && constraint;
                }

                versions[getVarNameForWalk(walk_init_index,varIndex)]++;
                z3::expr assignment = c.int_const((getVarNameForWalk(walk_init_index,varIndex) + "_" + std::to_string(versions[getVarNameForWalk(walk_init_index,varIndex)])).c_str());
                constraint = (assignment == sum);
                // solver.add(constraint);
                finalGeneratedConstraint = finalGeneratedConstraint && constraint;
                // solver.add(makeCoefficientsInteresting(coeffs, c));
                finalGeneratedConstraint = finalGeneratedConstraint && makeCoefficientsInteresting(coeffs, c);
                statementIndex++;
            }
            //now, we need to generate the conditional constraint
            logFile << blockTargets.size()<<'\n';
            logFile << "Target: "<<target<<'\n';
            if(blockTargets.size() > 1){
                z3::expr finalConstraint = c.bool_val(true);
                for(int i = 0; i< blockTargets.size() - 1; i++)
                {
                    //we need to have a separate constraint for each target, and for taking the kth target, all constraints from 0 to k-1 should be false, and the kth constraint should be true
                    std::vector<z3::expr> terms;
                    // Define integer variables and coefficients
                    std::vector<z3::expr> vars;
                    std::vector<z3::expr> coeffs;
                    int ctr = 0;
                    for(auto var_index: conditionalVariables)
                    {
                        vars.push_back(c.int_const((getVarNameForWalk(walk_init_index,var_index) + "_" + std::to_string(versions[getVarNameForWalk(walk_init_index,var_index)])).c_str()));
                        // solver.add(vars.back() >= LOWER_BOUND && vars.back() <= UPPER_BOUND);
                        finalGeneratedConstraint = finalGeneratedConstraint && (vars.back() >= LOWER_BOUND && vars.back() <= UPPER_BOUND);
                        coeffs.push_back(c.int_const((generateConditionalCoefficientName(blockno, 0, ctr)).c_str()));
                        // solver.add(coeffs.back() >= LOWER_BOUND && coeffs.back() <= UPPER_BOUND);
                        parameters[generateConditionalCoefficientName(blockno, 0, ctr)] = std::nullopt;
                        terms.push_back(coeffs.back() * vars.back()); 
                        ++ctr;
                    }
                    // solver.add(boundCoefficients(c, coeffs));
                    // solver.add(makeCoefficientsInteresting(coeffs, c));
                    finalGeneratedConstraint = finalGeneratedConstraint && boundCoefficients(c, coeffs);
                    finalGeneratedConstraint = finalGeneratedConstraint && makeCoefficientsInteresting(coeffs, c);
                    terms.push_back(c.int_const((generateConditionalConstantName(blockno, blockTargets[0])).c_str()));
                    parameters[generateConditionalConstantName(blockno, blockTargets[0])] = std::nullopt;
                    coeffs.push_back(c.int_const((generateConditionalConstantName(blockno, blockTargets[0])).c_str()));
                    // solver.add(coeffs.back() >= LOWER_BOUND && coeffs.back() <= UPPER_BOUND);
                    finalGeneratedConstraint = finalGeneratedConstraint && (coeffs.back() >= LOWER_BOUND && coeffs.back() <= UPPER_BOUND);
                    z3::expr sum = terms[0];
                    z3::expr term_constraint = (terms[0] <= UPPER_BOUND) && (terms[0] >= LOWER_BOUND);
                    if(enableSafetyChecks) {
                        // solver.add(term_constraint);
                        finalGeneratedConstraint = finalGeneratedConstraint && term_constraint;
                    }
                    for (int i = 1; i < terms.size(); i++) {
                        z3::expr constraint = (sum <= UPPER_BOUND) && (sum >= LOWER_BOUND);
                        if(enableSafetyChecks) 
                        {
                            // solver.add(constraint);
                            finalGeneratedConstraint = finalGeneratedConstraint && constraint;
                        }
                        sum = sum + terms[i];
                        term_constraint = (terms[i] <= UPPER_BOUND) && (terms[i] >= LOWER_BOUND);
                        if(enableSafetyChecks) {
                            // solver.add(term_constraint);
                            finalGeneratedConstraint = finalGeneratedConstraint && term_constraint;
                        }
                    }
                    z3::expr constraint = (sum <= UPPER_BOUND) && (sum >= LOWER_BOUND);
                    if(enableSafetyChecks) {
                        // solver.add(constraint);
                        finalGeneratedConstraint = finalGeneratedConstraint && constraint;
                    }
                    if(target == blockTargets[0])
                    {
                        constraint = (sum >= 0);
                    }
                    else
                    {
                        constraint = (sum < 0);
                    }
    
                }

                // solver.add(constraint);
                finalGeneratedConstraint = finalGeneratedConstraint && constraint;
            }
            return finalGeneratedConstraint;
        }

};

void extractParametersFromModel(z3::model &model, z3::context &ctx)
{
    for (const auto& param : parameters) {
        const std::string& name = param.first;
        
        if (model.has_interp(ctx.int_const(name.c_str()).decl())) {
            z3::expr value = model.get_const_interp(ctx.int_const(name.c_str()).decl());
            
            // For int values
            if (value.is_numeral()) {
                int int_value;
                if (Z3_get_numeral_int(ctx, value, &int_value)) {
                    parameters[name] = int_value;
                    logFile << "Parameter " << name << " is in the model with value: " 
                              << int_value << '\n';
                }
            }
        } else {
            logFile << "Parameter " << name << " is not explicitly defined in the model\n";
        }
    }

}
z3::expr getAllNonVariablesFromModel(z3::model &model, z3::context &ctx)
{
    z3::expr fixAllNonVariables = ctx.bool_val(true);
    for (const auto& param : parameters) {
        const std::string& name = param.first;
        if(name.substr(0, 3) == "var" || name.substr(0, 4) == "init")
        {
            continue;
        }
        if (model.has_interp(ctx.int_const(name.c_str()).decl())) {
            z3::expr value = model.get_const_interp(ctx.int_const(name.c_str()).decl());
            fixAllNonVariables = fixAllNonVariables && (ctx.int_const(name.c_str()) == value);
        }
    }
    return fixAllNonVariables;
}


void dumpCodeToFile(std::vector<BB> basicBlocks, const std::string& filename, const std::string& code, const std::vector<std::vector<int>> &initialisations) 
{
    std::ofstream outputFile(filename);
    // first let's define a function that computes a checksum over var_1, var_2, ..., var_n where n is the number of variables
    outputFile << "#include <stdio.h>\n";
    outputFile << "#include <stdint.h>\n";
    outputFile << "const int mod = 1000000007;\n";
    outputFile << "static uint32_t crc32_tab[256];\nstatic uint32_t crc32_context = 0xFFFFFFFFUL;\nstatic void crc32_gentab (void)\n{\tuint32_t crc;\tconst uint32_t poly = 0xEDB88320UL;\tint i, j;\n\tfor (i = 0; i < 256; i++) {\t\tcrc = i;\t\tfor (j = 8; j > 0; j--) {\t\t\tif (crc & 1) {\t\t\t\tcrc = (crc >> 1) ^ poly;\t\t\t} else {\t\t\t\tcrc >>= 1;\t\t\t}\t\t}\t\tcrc32_tab[i] = crc;\t}\n}\n\nstatic void crc32_byte (uint8_t b) {\tcrc32_context = ((crc32_context >> 8) & 0x00FFFFFF) ^ crc32_tab[(crc32_context ^ b) & 0xFF];\n}\n\nstatic void crc32_4bytes (uint32_t val)\n{\tcrc32_byte ((val>>0) & 0xff);\tcrc32_byte ((val>>8) & 0xff);\tcrc32_byte ((val>>16) & 0xff);\tcrc32_byte ((val>>24) & 0xff);\n}\n";
    outputFile << "void computeChecksum(\n";
    for(int i = 0; i < NUM_VARS; i++)
    {
        outputFile << "int " << getVarName(i);
        if(i != NUM_VARS - 1)
        {
            outputFile << ", ";
        }
    }
    outputFile << ")\n";
    outputFile << "{\n";
    //unroll the loop for computing checksum
    for(int i = 0; i < NUM_VARS; i++)
    {
        outputFile << "    crc32_4bytes(" << getVarName(i) << ");\n";
    }
    outputFile << "}\n";
    outputFile <<  "void gotoFunction(";
    for(int i = 0; i < NUM_VARS; i++)
    {
        outputFile << "int " << getVarName(i);
        
            outputFile << ", ";
    }
    outputFile << "int walk_type)\n";
    outputFile << "{\n";
    for (int i = 0; i < NODES; i++) {
        outputFile << "    " << basicBlocks[i].generateCode();
    }
    outputFile << "}\n";
    outputFile << "int main() {\n";
    outputFile << "crc32_gentab();\n";
    for(int i = 0; i < initialisations.size(); i++)
    {
        outputFile << "gotoFunction(";

        for(int j = 0; j < NUM_VARS; j++)
        {
            outputFile << initialisations[i][j] << ", ";
        }
        outputFile << i << ");\n";
    }
    outputFile << "    printf(\"%d\", crc32_context);\n";
    outputFile << "    return 0;\n";
    outputFile << "}\n";
    outputFile.close();
    logFile << "Code has been written to " << filename << '\n';    
    }
//write a signal handler to handle SIGINT and SIGTERM signals
void signalHandler(int signum) {
    logFile << "Interrupt signal (" << signum << ") received.\n";
    if(isFormulaSatisfiable)
    {
        return;
    }
    else
    {
        logFile << "Exiting gracefully..." << '\n';
        logFile.close();
        exit(signum);
    }
}
void sigKillHandler(int signum) {
        logFile << "Exiting ON SIGKILL..." << '\n';
        logFile.close();
        exit(signum);
}
std::string generateInitialisationName(int walk_ctr, int initialisation_ctr, int var_index)
{
    return "init_" + std::to_string(walk_ctr) + "_" + std::to_string(initialisation_ctr) + "_" + std::to_string(var_index);
}
z3::expr generateInitialisationEqualityConstraints(int walk_ctr, int initialisation_ctr, z3::context& c)
{
    int walk_init_index = walk_ctr * NUMBER_OF_INITIALISATIONS_OF_EACH_WALK + initialisation_ctr;
    z3::expr allAssignedConstraint = c.bool_val(true);
    for (int i = 0; i < NUM_VARS; i++) 
    {
        allAssignedConstraint = allAssignedConstraint && (c.int_const((getVarNameForWalk(walk_init_index, i)+ "_0").c_str()) == c.int_const((generateInitialisationName(walk_ctr, initialisation_ctr, i)).c_str()));
        parameters[generateInitialisationName(walk_ctr, initialisation_ctr, i)] = std::nullopt;
    }
    return allAssignedConstraint;
}
z3::expr generateInitialisationBoundnessConstraints(int walk_ctr, int initialisation_ctr, z3::context& c)
{
    z3::expr allAssignedConstraint = c.bool_val(true);
    for (int i = 0; i < NUM_VARS; i++) 
    {
        z3::expr boundedValue =  c.int_const((generateInitialisationName(walk_ctr, initialisation_ctr, i)).c_str());
        allAssignedConstraint = allAssignedConstraint && (boundedValue >= LOWER_INIT_BOUND && boundedValue <= UPPER_INIT_BOUND);
    }
    return allAssignedConstraint;
}
z3::expr generateDifferentInitialisationConstraints(int walk_ctr, int initialisation_ctr_1, int initialisation_ctr_2, z3::context& c)
{
    //there should be atlease NUM_VAR/2 variables which are not equal in both initialisations

    std::vector<z3::expr> differentInitialisationConstraints;
    for (int i = 0; i < NUM_VARS; i++) 
    {
        z3::expr boundedValue1 =  c.int_const((generateInitialisationName(walk_ctr, initialisation_ctr_1, i)).c_str());
        z3::expr boundedValue2 =  c.int_const((generateInitialisationName(walk_ctr, initialisation_ctr_2, i)).c_str());
        differentInitialisationConstraints.push_back(z3::ite(boundedValue1 != boundedValue2, c.int_val(1), c.int_val(0)));
    }
    z3::expr differentInitialisationConstraint = atMostKZeroes(c, differentInitialisationConstraints, min(2, NUM_VARS/2), 0);
    return differentInitialisationConstraint;
}
int main(int argc, char** argv)
{
    //program should have a command line argument which we will store in an integer sample_number
    int sample_number = 0;
    if (argc > 1) {
        sample_number = std::stoi(argv[1]);
    }
    else
    {
        return 1;
    }
    logFile = std::ofstream("logs/log_" + std::to_string(sample_number) + ".txt");
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, sigKillHandler);
    std::signal(SIGKILL, sigKillHandler);

    logFile << "Sample number: " << sample_number << '\n';
    z3::set_param("parallel.enable", true);

    int  nodes = NODES; // Number of nodes
    Graph g(nodes);
    g.generate_graph();
    std::vector<std::set<int>> adjacency_list = g.adj;
    std::vector<BB> basicBlocks;
    for(int i = 0; i < nodes; i++)
    {
        BB bb(i, adjacency_list[i]);
        basicBlocks.push_back(bb);
    }
    std::cout<<"generated graph successfully\n";

    g.print_graph();

    
    std::vector<std::vector<int>> sample_walks = {};
    if(enableConsistentWalks)
    {
        sample_walks = g.get_k_distinct_walks(0, nodes - 1, NUMBER_OF_DISTINCT_WALKS);
    }
    else
    {
        sample_walks = g.get_k_distinct_consistent_walks(0, nodes - 1, NUMBER_OF_DISTINCT_WALKS);
    }
    for (auto sample_walk : sample_walks)
    {
        logFile << "Sample walk: ";
        for(auto x: sample_walk)
        {
            logFile << x<<", ";
        }
        logFile << std::endl;
    }
    int walk_ctr = 0;
    for(auto& sample_walk: sample_walks)
    {
        if(sample_walk[sample_walk.size() - 1] != NODES - 1)
        {
            // modify the basic block at the end of the sample walk to have pass counter equal to the number of times that basic block has been visited
            int passCounter = 0;
            for(int i = 0; i < sample_walk.size(); i++)
            {
                if(sample_walk[i] == sample_walk[sample_walk.size() - 1])
                {
                    passCounter++;
                }
            }
            basicBlocks[sample_walk[sample_walk.size() - 1]].setPassCounter(passCounter, walk_ctr);
            parameters[generatePassCounterName(sample_walk[sample_walk.size() - 1])] = passCounter;
            sample_walk.push_back(NODES - 1);
            logFile << "Sample walk has been modified to end at the last node." << '\n';
        }
        walk_ctr++;
    }
    std::cout<<"generated sample walks successfully\n";
    z3::context c;
    z3::solver solver(c);
    z3::expr previous_coefficients_constraint = c.bool_val(true);
    z3::expr previous_initialisations_constraint = c.bool_val(true);
    std::vector<std::vector<int>> initialisations;
    walk_ctr = 0;
    for (auto& sample_walk : sample_walks){
        //add common constraints for all initialisations
        

        // we now model initialisations as uninterpreted constants, and have a constrain like  (var_0 == init_0_0 && var_1 == init_0_1 && ... && var_n == init_0_n) => this_walk_taken_constraint
        // where init_i_j is the initialisation of variable j in the ith sample walk
        // we also need to add a constraint that the initialisation of each variable is between LOWER_INIT_BOUND and UPPER_INIT_BOUND
        // and that the initialisation of each variable is not equal to 0
        for(int i = 0; i< NUMBER_OF_INITIALISATIONS_OF_EACH_WALK; i++)
        {   
            std::unordered_map<std::string, int> versions;
            int walk_init_index = walk_ctr * NUMBER_OF_INITIALISATIONS_OF_EACH_WALK + i;
            z3::expr this_walk_taken_constraint = c.bool_val(true);
            for(int j = 0; j < sample_walk.size() - 1; j++)
            {
                int current_bb = sample_walk[j];
                int next_bb = sample_walk[j + 1];
                this_walk_taken_constraint = this_walk_taken_constraint && basicBlocks[current_bb].generateConstraints(next_bb, solver, c, versions, walk_init_index);
            }
            this_walk_taken_constraint = this_walk_taken_constraint && basicBlocks[sample_walk[sample_walk.size() - 1]].generateConstraints(-1, solver, c, versions, walk_init_index); 
            z3::expr antecedent_constraint = generateInitialisationBoundnessConstraints(walk_ctr, i, c) && generateInitialisationEqualityConstraints(walk_ctr, i, c);
            solver.add(antecedent_constraint && this_walk_taken_constraint);
            for(int j = i + 1; j < NUMBER_OF_INITIALISATIONS_OF_EACH_WALK; j++)
            {
                if(i != j)
                {
                    solver.add(generateDifferentInitialisationConstraints(walk_ctr, i, j, c));
                }
            }
        }
        walk_ctr++;
    }
    logFile << "Query:\n" << solver.assertions() << std::endl;

    std::cout<<"added all constraints successfully\n";

    //now check if the formula is satisfiable
    if (solver.check() == z3::sat) {
        isFormulaSatisfiable = true;
        logFile << "Formula is satisfiable"<<std::endl;
        z3::model model = solver.get_model();
        extractParametersFromModel(model, c);
        //print the model
        // logFile << "Model:\n" << model << std::endl;
        // logFile << "Query:\n" << solver.assertions() << std::endl;
        // z3::expr fixAllNonVariables = getAllNonVariablesFromModel(model, c);
        // z3::expr fixAllInitialVariables = getInitialVariablesFromModel(model, c);
        // solver.add(fixAllNonVariables);
        // solver.add(fixAllInitialVariables);
        if (solver.check() == z3::sat) {
            logFile << "Model is satisfiable\n";
            model = solver.get_model();
            extractParametersFromModel(model, c);
            for(int i = 0; i < sample_walks.size(); i++)
            {
                for(int j = 0; j < NUMBER_OF_INITIALISATIONS_OF_EACH_WALK; j++)
                {
                    std::vector<int> initialisation;
                    for(int k = 0; k < NUM_VARS; k++)
                    {
                        try{
                            initialisation.push_back(parameters[generateInitialisationName(i, j, k)].value());
                        }
                        catch(const std::exception& e)
                        {
                            // logFile << "Error: " << e.what() << '\n';
                            //push a random value between LOWER_INIT_BOUND and UPPER_INIT_BOUND
                            std::random_device rd;
                            std::mt19937 gen(rd());
                            std::uniform_int_distribution<int> dist(LOWER_INIT_BOUND, UPPER_INIT_BOUND);
                            initialisation.push_back(dist(gen));
                        }
                    }
                    initialisations.push_back(initialisation);
                }
            }
        }
    } else {

        logFile << "Formula is unsatisfiable\n";
        logFile << "Query:\n" << solver.assertions() << std::endl;
        exit(1);
    }
    // Dump the generated code to a file
    std::string filename = "inlinable/generated_code_" + std::to_string(sample_number) + ".c";
    dumpCodeToFile(basicBlocks, filename, "", initialisations);
    // z3::expr_vector assertions = solver.assertions();
    // logFile << "Method 1 - Z3 assertions:\n" << assertions << '\n';
    for (auto sample_walk : sample_walks)
    {
        logFile << "Sample walk: ";
        for(auto x: sample_walk)
        {
            logFile << x<<", ";
        }
        logFile << '\n';
    }
    // Dump the generated code to a file
    logFile << "C code has been generated and written to 'generated_code.c'." << '\n';
    logFile.close();
    return 0;
}