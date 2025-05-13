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
std::vector<std::pair<std::string, int>> interProceduralMappings;
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
std::string generateConditionalConstraint(int blockno, int target, std::vector<VarIndex> conditionalVariables, std::unordered_map<std::string, std::optional<int>> &local_parameters)
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
        if (local_parameters.find(coefficientKey) != local_parameters.end() && local_parameters[coefficientKey].has_value())
        {
            coefficient = local_parameters[coefficientKey].value();
        }
        else
        {
            coefficient = dist(gen);
            local_parameters[coefficientKey] = coefficient;
        }
        constraint << coefficient << " * " << getVarName(i);
        constraint << " + ";
        ++ctr;
    }
    std::string constantKey = generateConditionalConstantName(blockno, target);
    int constant;
    if (local_parameters.find(constantKey) != local_parameters.end() && local_parameters[constantKey].has_value())
    {
        constant = local_parameters[constantKey].value();
    }
    else
    {
        constant = dist(gen);
        local_parameters[constantKey] = constant;
    }
    constraint  << constant;
    constraint << " >= 0) goto " << generateLabelName(target) << ";\n";
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
    auto expr =  atMostKZeroes(c, coeffs, coeffs.size()/2, 0);
    //we also want that some of the coefficients are different from each other
    // for(int i = 0; i < coeffs.size(); i++)
    // {
    //     //do the following with a 33% probability
    //     if(std::rand() % 5 == 0)
    //     {
    //         int j = std::rand() % coeffs.size();
    //         while(i == j)
    //         {
    //             j = std::rand() % coeffs.size();
    //         }
    //         expr = expr && ((coeffs[i] != coeffs[j]));
    //     }
        
    // }
    return expr;
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
    std::vector<VarIndex> conditionalVariables;
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
            logFile << "size of conditional variables: "<<conditionalVariables.size()<<'\n';

            std::vector<BasicBlockNo> targets;
            for(auto target: graphTargets)
            {
                blockTargets.push_back(target);
            }
            if(blockTargets.size() > 1)
            {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::shuffle(blockTargets.begin(), blockTargets.end(), gen);
                for(int i = 0; i < NUM_VARIABLES_IN_CONDITIONAL; i++)
                {
                    std::string coefficientKey = generateConditionalCoefficientName(blockno, 0, i);
                    parameters[coefficientKey] = std::nullopt;
                }
                std::string constantKey = generateConditionalConstantName(blockno, blockTargets[0]);
                parameters[constantKey] = std::nullopt;
            }
        }
        void setPassCounter(int value, int walkno)
        {
            needPassCounter = true;
            passCounterValues.push_back(std::make_pair(value, walkno));
        }
        std::string generateCode(std::unordered_map<std::string, std::optional<int>> &local_parameters) const
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
                    // Look up the coefficient in the local_parameters map
                    std::string coefficientKey = getCoefficientName(blockno, statementIndex, i);
                    int coefficient;
                    if (local_parameters.find(coefficientKey) != local_parameters.end() && local_parameters[coefficientKey].has_value())
                    {
                        coefficient = local_parameters[coefficientKey].value();
                        // let's do this coefficient OR a procedure call by sampling from interProceduralMappings
                        if(std::rand() % 2 == 0 && !interProceduralMappings.empty())
                        {
                            //random value from interProceduralMappings
                            int randomIndex = std::rand() % interProceduralMappings.size();
                            logFile << "sampled random index: " << randomIndex << " now outputting the coefficient: " << interProceduralMappings[randomIndex].first << '\n';
                            code << "(" << interProceduralMappings[randomIndex].first << " + " << std::to_string(coefficient - interProceduralMappings[randomIndex].second) << ")";
                        }
                        else{
                            logFile << "present in map but we's rather use the coefficient: " << coefficient << "for coefficientKey: " << coefficientKey << '\n';
                            code << coefficient;
                        }
                        code << " * " << getVarName(dependencies[i]);
                    }
                    else
                    {
                        //let's do a random number OR a procedure call 
                        if(std::rand() % 2 == 1 || interProceduralMappings.empty())
                        {
                            //random value
                            coefficient = dist(gen);
                            logFile << "not present in map and we'd rather use the coefficient: " << coefficient << "for coefficientKey: " << coefficientKey << '\n';
                            code << coefficient << " * " << getVarName(dependencies[i]);
                        }
                        else
                        {
                            //call a procedure
                            int procedureIndex = std::rand() % NUMBER_OF_PROCEDURES;
                            logFile << "not present in map but we'd rather use the procedure mapping: for coefficientKey: " << coefficientKey << '\n';

                            code << "proc_" + std::to_string(procedureIndex) << "(";
                            //random values 
                            for(int j = 0; j < NUM_VARS; j++)
                            {
                                code << dist(gen);
                                if(j < NUM_VARS - 1)
                                {
                                    code << ",";
                                }
                            }
                            code << ")";
                            code << " * " << getVarName(dependencies[i]);
                            coefficient = 1; // Set a default coefficient for the procedure call
                        }
                        local_parameters[coefficientKey] = coefficient; // Store it in local_parameters
                    }

                    if (i < dependencies.size() - 1)
                    {
                        code << " + "; // Example operation
                    }
                }

                // Add a constant term
                std::string constantKey = generateConstantName(blockno, statementIndex);
                int constant;
                if (local_parameters.find(constantKey) != local_parameters.end() && local_parameters[constantKey].has_value())
                {
                    constant = local_parameters[constantKey].value();
                }
                else
                {
                    constant = dist(gen); 
                    local_parameters[constantKey] = constant; 
                }
                code << " + " << constant << ";\n";

                ++statementIndex;
            }
            if(needPassCounter)
            {
                for(auto passCounterValue: passCounterValues)
                {
                    code << "    if(" << generatePassCounterName(blockno) << " == " << passCounterValue.first << ") goto " << generateLabelName(NODES - 1) << ";\n";
                }
            }
            if(blockTargets.size() > 1)
            {
                code << generateConditionalConstraint(blockno, blockTargets[0], conditionalVariables, local_parameters);
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
                code << "    return computeContextFreeChecksum(";
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
                    if(parameters.find(getCoefficientName(blockno, statementIndex, i)) != parameters.end() && parameters[getCoefficientName(blockno, statementIndex, i)].has_value())
                    {
                        // std::cout<<getCoefficientName(blockno, statementIndex, i)<<" "<<parameters[getCoefficientName(blockno, statementIndex, i)].value()<<'\n';
                        coeffs.push_back(c.int_val(parameters[getCoefficientName(blockno, statementIndex, i)].value()));
                    }
                    else if (parameters.find(getCoefficientName(blockno, statementIndex, i)) != parameters.end() && parameters[getCoefficientName(blockno, statementIndex, i)] == std::nullopt)
                    {
                        coeffs.push_back(c.int_const((getCoefficientName(blockno, statementIndex, i)).c_str()));
                        parameters[getCoefficientName(blockno, statementIndex, i)] = std::nullopt;
                    }
                    else if (parameters.find(getCoefficientName(blockno, statementIndex, i)) == parameters.end())
                    {
                        coeffs.push_back(c.int_const((getCoefficientName(blockno, statementIndex, i)).c_str()));
                        parameters[getCoefficientName(blockno, statementIndex, i)] = std::nullopt;
                    }
                    // coeffs.push_back(c.int_const((getCoefficientName(blockno, statementIndex, i)).c_str()));
                    terms.push_back(coeffs.back() * vars.back()); // a_i * var_i
                }
                // solver.add(boundCoefficients(c, coeffs));
                finalGeneratedConstraint = finalGeneratedConstraint && boundCoefficients(c, coeffs);
                if(parameters.find(generateConstantName(blockno, statementIndex)) != parameters.end() && parameters[generateConstantName(blockno, statementIndex)].has_value())
                {
                    coeffs.push_back(c.int_val(parameters[generateConstantName(blockno, statementIndex)].value()));
                }
                else if (parameters.find(generateConstantName(blockno, statementIndex)) != parameters.end() && parameters[generateConstantName(blockno, statementIndex)] == std::nullopt)
                {
                    coeffs.push_back(c.int_const((generateConstantName(blockno, statementIndex)).c_str()));
                    parameters[generateConstantName(blockno, statementIndex)] = std::nullopt;
                }
                else if (parameters.find(generateConstantName(blockno, statementIndex)) == parameters.end())
                {
                    coeffs.push_back(c.int_const((generateConstantName(blockno, statementIndex)).c_str()));
                    parameters[generateConstantName(blockno, statementIndex)] = std::nullopt;
                }
                terms.push_back(coeffs.back());
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
                logFile << "Generating conditional constraint for block: "<<blockno<<'\n';
                std::vector<z3::expr> terms;
                // Define integer variables and coefficients
                std::vector<z3::expr> vars;
                std::vector<z3::expr> coeffs;
                int ctr = 0;
                for(auto i: conditionalVariables)
                {
                    vars.push_back(c.int_const((getVarNameForWalk(walk_init_index,i) + "_" + std::to_string(versions[getVarNameForWalk(walk_init_index,i)])).c_str()));
                    // solver.add(vars.back() >= LOWER_BOUND && vars.back() <= UPPER_BOUND);
                    finalGeneratedConstraint = finalGeneratedConstraint && (vars.back() >= LOWER_BOUND && vars.back() <= UPPER_BOUND);
                    if(parameters.find(generateConditionalCoefficientName(blockno, 0, ctr)) != parameters.end() && parameters[generateConditionalCoefficientName(blockno, 0, ctr)].has_value())
                    {
                        coeffs.push_back(c.int_val(parameters[generateConditionalCoefficientName(blockno, 0, ctr)].value()));
                    }
                    else if (parameters.find(generateConditionalCoefficientName(blockno, 0, ctr)) != parameters.end() && parameters[generateConditionalCoefficientName(blockno, 0, ctr)] == std::nullopt)
                    {
                        logFile<<"pushing back "<<generateConditionalCoefficientName(blockno, 0, ctr)<<'\n';
                        coeffs.push_back(c.int_const((generateConditionalCoefficientName(blockno, 0, ctr)).c_str()));
                        parameters[generateConditionalCoefficientName(blockno, 0, ctr)] = std::nullopt;
                    }
                    else if (parameters.find(generateConditionalCoefficientName(blockno, 0, ctr)) == parameters.end())
                    {
                        coeffs.push_back(c.int_const((generateConditionalCoefficientName(blockno, 0, ctr)).c_str()));
                        parameters[generateConditionalCoefficientName(blockno, 0, ctr)] = std::nullopt;
                    }
                    // coeffs.push_back(c.int_const((generateConditionalCoefficientName(blockno, 0, ctr)).c_str()));
                    // // solver.add(coeffs.back() >= LOWER_BOUND && coeffs.back() <= UPPER_BOUND);
                    // parameters[generateConditionalCoefficientName(blockno, 0, ctr)] = std::nullopt;
                    terms.push_back(coeffs.back() * vars.back()); 
                    ++ctr;
                }
                // solver.add(boundCoefficients(c, coeffs));
                // solver.add(makeCoefficientsInteresting(coeffs, c));
                finalGeneratedConstraint = finalGeneratedConstraint && boundCoefficients(c, coeffs);
                finalGeneratedConstraint = finalGeneratedConstraint && makeCoefficientsInteresting(coeffs, c);
                if(parameters.find(generateConditionalConstantName(blockno, blockTargets[0])) != parameters.end() && parameters[generateConditionalConstantName(blockno, blockTargets[0])].has_value())
                {
                    coeffs.push_back(c.int_val(parameters[generateConditionalConstantName(blockno, blockTargets[0])].value()));
                }
                else if (parameters.find(generateConditionalConstantName(blockno, blockTargets[0])) != parameters.end() && parameters[generateConditionalConstantName(blockno, blockTargets[0])] == std::nullopt)
                {
                    coeffs.push_back(c.int_const((generateConditionalConstantName(blockno, blockTargets[0])).c_str()));
                    parameters[generateConditionalConstantName(blockno, blockTargets[0])] = std::nullopt;
                }
                else if (parameters.find(generateConditionalConstantName(blockno, blockTargets[0])) == parameters.end())
                {
                    coeffs.push_back(c.int_const((generateConditionalConstantName(blockno, blockTargets[0])).c_str()));
                    parameters[generateConditionalConstantName(blockno, blockTargets[0])] = std::nullopt;
                }
                terms.push_back(coeffs.back());
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
                    logFile << "Target: "<<target<<'\n';
                    constraint = (sum >= 0);
                    logFile<< "Target constraint is "<<constraint.to_string()<<'\n';
                    
                }
                else
                {
                    logFile << "Target: "<<blockTargets[1]<<'\n';
                    constraint = (sum < 0);
                    logFile<< "Target constraint is "<<constraint.to_string()<<'\n';
                    
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
                    assert(! parameters[name].has_value() && "Parameter should NOT be in the model");
                    parameters[name] = int_value;
                    logFile << "Parameter " << name << " is in the model with value: " 
                              << int_value << '\n';
                }
            }
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
    z3::expr differentInitialisationConstraint = atMostKZeroes(c, differentInitialisationConstraints, std::min(3, NUM_VARS/2), 0);
    return differentInitialisationConstraint;
}
// z3::expr evaluateContextFreeChecksum(z3::context& c, z3::model &m, std::vector<std::string> &final_variable_version_names)
// {
//     z3::expr checksum = c.int_val(0);
//     for (const auto& varName : final_variable_version_names) {
//         if (m.has_interp(c.int_const(varName.c_str()).decl())) {
//             z3::expr value = m.get_const_interp(c.int_const(varName.c_str()).decl());
//             if (value.is_numeral()) {
//                 int int_value;
//                 if (Z3_get_numeral_int(c, value, &int_value)) {
//                     checksum = checksum + int_value;
//                 }
//             }
//         }
//     }
//     return checksum;
// }
z3::expr computeChecksum(z3::context& c, const std::vector<z3::expr>& vars) {
    //take a running sum of vars mod 1e9+7
    z3::expr checksum = c.int_val(0);
    for (const auto& var : vars) {
        checksum = checksum + var;
    }
    checksum = checksum % c.int_val(MODULO_PRIME);
    return checksum;
}
void addMappingToModel(std::string name, z3::model &model, z3::context &ctx, std::vector<int> initialisation, std::vector<std::string> final_variable_versions)
{
    std::vector<z3::expr> final_vars;
    for(auto x: final_variable_versions)
    {
        final_vars.push_back(ctx.int_const(x.c_str()));
    }
    z3::expr evaluateFinalChecksum = computeChecksum(ctx, final_vars);
    auto model_answer = model.eval(evaluateFinalChecksum, true);
    //get the value of the checksum
    int checksumValue;
    if (Z3_get_numeral_int(ctx, model_answer, &checksumValue)) {
        logFile << "Checksum value: " << checksumValue << '\n';
    } else {
        logFile << "Failed to get checksum value\n";
    }
    //now add this to the interProceduralMappings
    std::string function_call_name = name + "(";
    for(int k = 0; k < NUM_VARS; k++)
    {
        function_call_name += std::to_string(initialisation[k]);
        if(k != NUM_VARS - 1)
        {
            function_call_name += ", ";
        }
    }
    function_call_name += ")";
    interProceduralMappings.push_back({function_call_name, checksumValue});
}
class Procedure
{
    public:
        std::string name;
        std::vector<BB> basicBlocks;
        Graph g;
        std::vector<int> sample_walk;
        std::vector<std::pair<std::vector<int>, std::vector<std::string>>> initialisations_and_final_versions;
        std::unordered_map<std::string, std::optional<int>> local_procedural_parameters;
        Procedure(std::string name, Graph &g) : name(name), g(g) {
            int nodes = NODES; // Number of nodes
            for(int i = 0; i < nodes; i++)
            {
                BB bb(i, g.adj[i]);
                basicBlocks.push_back(bb);
            }
            if(enableConsistentWalks)
            {
                sample_walk = g.sample_walk(0, nodes - 1);
            }
            else
            {
                sample_walk = g.sample_consistent_walk(0, nodes - 1);
            }
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
                basicBlocks[sample_walk[sample_walk.size() - 1]].setPassCounter(passCounter, 0);
                parameters[generatePassCounterName(sample_walk[sample_walk.size() - 1])] = passCounter;
                sample_walk.push_back(NODES - 1);
                logFile << "Sample walk has been modified to end at the last node." << '\n';
            }

            //print sample walk with procedure name
            logFile << "Sample walk for procedure " << name << ": ";
            for (const auto& node : sample_walk) {
                logFile << node << " ";
            }
            logFile << '\n';
        }
        void generateConstraints(z3::solver& solver, z3::context& c, std::unordered_map<std::string, int> &versions)
        {
            int walk_ctr = 0;
            
            int walk_init_index = walk_ctr * NUMBER_OF_INITIALISATIONS_OF_EACH_WALK;
            for(int i = 0; i < sample_walk.size() - 1; i++)
            {
                int current_bb = sample_walk[i];
                int next_bb = sample_walk[i + 1];
                solver.add(basicBlocks[current_bb].generateConstraints(next_bb, solver, c, versions, walk_init_index));
            }
            //add the last basic block to the solver
            solver.add(basicBlocks[sample_walk[sample_walk.size() - 1]].generateConstraints(-1, solver, c, versions, walk_init_index));
            solver.add(generateInitialisationEqualityConstraints(walk_ctr, 0, c) && generateInitialisationBoundnessConstraints(walk_ctr, 0, c));
            solver.add(makeInitialisationsInteresting(c, 0));
        }
        void solveForProcedureParameters(z3::solver& solver, z3::context& c)
        {
            std::unordered_map<std::string, int> versions;
            generateConstraints(solver, c, versions);
            if(solver.check() == z3::unsat)
            {
                logFile << "Formula is unsatisfiable\n";
                logFile << "Query:\n" << solver.assertions() << std::endl;
                exit(1);
            }
            else
            {
                z3::model model= solver.get_model();
                extractParametersFromModel(model, c);
                solver.reset();
                logFile << "Formula is satisfiable\n"<<std::endl;
                //now using these coefficients, generate another set of initialisations
                std::vector<int> initialisation;
                for(int i = 0; i < NUM_VARS; i++)
                {
                    initialisation.push_back(parameters[generateInitialisationName(0, 0, i)].value());
                }
                //now get the latest version of all variables from the model
                std::vector<int> latestVersions;
                std::vector<std::string> final_variable_versions_names;
                for(int i = 0; i < NUM_VARS; i++)
                {
                    final_variable_versions_names.push_back(getVarNameForWalk(0, i) + "_" + std::to_string(versions[getVarNameForWalk(0, i)]));
                }
                for(int i = 0; i < NUM_VARS; i++)
                {
                    std::string varName = getVarNameForWalk(0, i) + "_" + std::to_string(versions[getVarNameForWalk(0, i)]);
                    if(model.has_interp(c.int_const(varName.c_str()).decl()))
                    {
                        z3::expr value = model.get_const_interp(c.int_const(varName.c_str()).decl());
                        if(value.is_numeral())
                        {
                            int int_value;
                            if (Z3_get_numeral_int(c, value, &int_value)) {
                                latestVersions.push_back(int_value);
                            }
                        }
                    }
                    else{
                        logFile << "Variable " << varName << " is not in the model\n";
                        //initialise it with a random value 
                        std::random_device rd;
                        std::mt19937 gen(rd());
                        std::uniform_int_distribution<int> dist(LOWER_BOUND, UPPER_BOUND);
                        int randomValue = dist(gen);
                        latestVersions.push_back(randomValue);
                        logFile << "Variable " << varName << " is not in the model, assigning random value: " 
                                  << randomValue << '\n';
                    }
                }
                initialisations_and_final_versions.push_back({initialisation, final_variable_versions_names});
                // addMappingToModel(name, model, c, initialisation, latestVersions, final_variable_versions_names);
                //now we need to generate the next set of initialisations
                std::vector<std::vector<std::string>> final_variable_version_names;
                for(int i = 0; i < NUMBER_OF_INITIALISATIONS_OF_EACH_WALK; i++)
                {   
                    std::unordered_map<std::string, int> versions;
                    int walk_ctr = 0;
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
                    std::vector<std::string> final_variable_version_names_for_init;
                    for(int j = 0; j < NUM_VARS; j++)
                    {
                        final_variable_version_names_for_init.push_back(getVarNameForWalk(walk_init_index, j) + "_" + std::to_string(versions[getVarNameForWalk(walk_init_index, j)]));
                    }
                    final_variable_version_names.push_back(final_variable_version_names_for_init);
                }
                //now we need to check if the model is satisfiable
                logFile<<"dumping smt query to log file\n";
                logFile << solver.to_smt2() << std::endl;
                if(solver.check() == z3::unsat)
                {
                    logFile << "Formula is unsatisfiable\n";
                    logFile << "Query:\n" << solver.assertions() << std::endl;
                    exit(1);
                }
                else
                {
                    z3::model model= solver.get_model();
                    extractParametersFromModel(model, c);
                    int walk_ctr = 0;
                    for(int j = 0; j < NUMBER_OF_INITIALISATIONS_OF_EACH_WALK; j++)
                    {
                        std::vector<int> initialisation;
                        std::vector<int> final_variable_versions;
                        int walk_init_index = walk_ctr * NUMBER_OF_INITIALISATIONS_OF_EACH_WALK + j;
                        for(int k = 0; k < NUM_VARS; k++)
                        {
    
                            try{
                                initialisation.push_back(parameters[generateInitialisationName(0, j, k)].value());
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
                        initialisations_and_final_versions.push_back({initialisation, final_variable_version_names[j]});
                        logFile << " adding mapping to model" << std::endl;
                        // addMappingToModel(name, model, c, initialisation, final_variable_versions, final_variable_version_names[j]);
                        //add the mapping to the interProceduralMappings
                        logFile << "ADDED MAPPING TO MODEL" << std::endl;

                    }
                }
            }
            local_procedural_parameters = parameters;
        }
        void cleanup(z3::solver& solver)
        {
            //cleanup the local parameters
            local_procedural_parameters = parameters;
            parameters.clear();
            solver.reset();
        }
        std::string generateCode()
        {
            std::string code = "";
            code += "int " + name + "(";
            for(int i = 0; i < NUM_VARS; i++)
            {
                code += "int " + getVarName(i);
                if(i != NUM_VARS - 1)
                {
                    code += ", ";
                }
            }
            code += ")";
            code += "{\n";
            for(int i = 0; i < NODES; i++)
            {
                code  += "int " + generatePassCounterName(i) + " = 0;\n";
            }
            for (int i = 0; i < NODES; i++) {
                code += basicBlocks[i].generateCode(local_procedural_parameters);
            }
            code += "}\n";
            return code;
        }
        void addAllMappingsToModel(z3::model &model, z3::context &ctx)
        {
            for(auto &mapping: initialisations_and_final_versions)
            {
                std::vector<int> initialisation = mapping.first;
                std::vector<std::string> final_values = mapping.second;
                addMappingToModel(name, model, ctx, initialisation, final_values);
            }
        }
};










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
    std::ofstream outputFile("codegen_procedure_calls_output.c");
    outputFile << "#include <stdio.h>\n";
    outputFile << "#include <stdint.h>\n";
    outputFile << "const int mod = 1000000007;\n";
    outputFile << "static uint32_t crc32_tab[256];\nstatic uint32_t crc32_context = 0xFFFFFFFFUL;\nstatic void crc32_gentab (void)\n{\tuint32_t crc;\tconst uint32_t poly = 0xEDB88320UL;\tint i, j;\n\tfor (i = 0; i < 256; i++) {\t\tcrc = i;\t\tfor (j = 8; j > 0; j--) {\t\t\tif (crc & 1) {\t\t\t\tcrc = (crc >> 1) ^ poly;\t\t\t} else {\t\t\t\tcrc >>= 1;\t\t\t}\t\t}\t\tcrc32_tab[i] = crc;\t}\n}\n\nstatic void crc32_byte (uint8_t b) {\tcrc32_context = ((crc32_context >> 8) & 0x00FFFFFF) ^ crc32_tab[(crc32_context ^ b) & 0xFF];\n}\n\nstatic void crc32_4bytes (uint32_t val)\n{\tcrc32_byte ((val>>0) & 0xff);\tcrc32_byte ((val>>8) & 0xff);\tcrc32_byte ((val>>16) & 0xff);\tcrc32_byte ((val>>24) & 0xff);\n}\n";
    outputFile << "static uint32_t context_free_crc32_byte(uint32_t context, uint8_t b) {\treturn ((context >> 8) & 0x00FFFFFF) ^ crc32_tab[(context ^ b) & 0xFF];\n}\n\nstatic uint32_t context_free_crc32_4bytes(uint32_t context, uint32_t val) {\tcontext = context_free_crc32_byte(context, (val >> 0) & 0xFF);\tcontext = context_free_crc32_byte(context, (val >> 8) & 0xFF);\tcontext = context_free_crc32_byte(context, (val >> 16) & 0xFF);\tcontext = context_free_crc32_byte(context, (val >> 24) & 0xFF);\treturn context;\n}";
    outputFile << "void computeChecksum(\n";
    for(int i = 0; i < NUM_VARS; i++)
    {
        outputFile << "int " << getVarName(i);
        if(i != NUM_VARS - 1)
        {
            outputFile << ", ";
        }
    }
    z3::context c;
    z3::solver solver(c);
    outputFile << ")\n";
    outputFile << "{\n";
    //unroll the loop for computing checksum
    for(int i = 0; i < NUM_VARS; i++)
    {
        outputFile << "    crc32_4bytes(" << getVarName(i) << ");\n";
    }
    outputFile << "}\n";
    outputFile << "int computeContextFreeChecksum(\n";
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
    //add variables mod mod
    outputFile << " long running_sum = 0;\n";
    for(int i = 0; i < NUM_VARS; i++)
    {
        outputFile << "    running_sum = (running_sum + " << getVarName(i) << ") % mod;\n";
    }
    outputFile << "    running_sum = (running_sum + mod) % mod;\n";
    outputFile << "    return running_sum;\n";
    outputFile << "}\n";
    //dump declarations for all procedures to the output file
    for(int i = 0; i < NUMBER_OF_PROCEDURES; i++)
    {
        outputFile << "int proc_" + std::to_string(i) + "(";
        for(int j = 0; j < NUM_VARS; j++)
        {
            outputFile << "int " + getVarNameForWalk(0, j);
            if(j != NUM_VARS - 1)
            {
                outputFile << ", ";
            }
        }
        outputFile << ");\n";
    }
    //unroll the loop for computing checksum
    int  nodes = NODES; // Number of nodes
    Graph g(nodes);
    g.generate_graph();
    std::unordered_map<std::string, std::vector<std::pair<std::vector<int>, std::vector<std::string>>> > initialisations;
    for(int i = 0; i < NUMBER_OF_PROCEDURES; i++)
    {
        Procedure p("proc_" + std::to_string(i), g);
        p.solveForProcedureParameters(solver, c);
        
        std::string code = p.generateCode();
        //append the code to the output file
        outputFile << code;
        z3::model model = solver.get_model();
        p.addAllMappingsToModel(model, c);
        initialisations["proc_" + std::to_string(i)] = p.initialisations_and_final_versions;
        p.cleanup(solver);
        assert(parameters.size() == 0 && "Parameters should be empty after cleanup");

    }

    outputFile << "int main() {\n";
    outputFile << "crc32_gentab();\n";
    for(int i = 0; i < NUMBER_OF_PROCEDURES; i++)
    {
        
        for(auto init_final_ver_pair: initialisations["proc_" + std::to_string(i)])
        {
            outputFile << "proc_" + std::to_string(i) + "(";
            std::vector<int> initialisation = init_final_ver_pair.first;
            for(int j = 0; j < NUM_VARS; j++)
            {
                if(j == NUM_VARS - 1)
                {
                    outputFile << initialisation[j];
                }
                else
                {
                    outputFile << initialisation[j] << ", ";
                }
            }
            outputFile <<");\n";
        }
        
    }
    outputFile << "    printf(\"%d\", crc32_context);\n";
    outputFile << "    return 0;\n";
    outputFile << "}\n";
    outputFile.close();   
    
    //dump interprocedural mappings to logFile
    logFile << "Interprocedural mappings:\n";
    for(auto mapping: interProceduralMappings)
    {
        logFile << mapping.first << " -> " << mapping.second << '\n';
    }
    logFile.close();
    
}