#include <bits/stdc++.h>
#include "better_graph.hpp"
#include"z3++.h"
#include <optional>
using VarIndex = int;
using BasicBlockNo = int;
#include "params.hpp"
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
std::string getVarName(int varIndex)
{
    return "var_" + std::to_string(varIndex);
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
z3::expr makeInitialisationsInteresting(z3::context& c)
{
    if(!enableInterestingInitialisations)
    {
        return c.bool_val(true);
    }
    std::vector<z3::expr> allVars;
    for (int i = 0; i < NUM_VARS; i++) 
    {
        allVars.push_back(c.int_const((getVarName(i)+ "_0").c_str()));
    }
    return atMostKZeroes(c, allVars, NUM_VARS/2, 0);
}
class BB{
    BasicBlockNo blockno;
    int numStatements;
    std::map<VarIndex, std::vector<VarIndex>> statementMappings;
    std::vector<VarIndex> assignmentOrder;
    std::vector<BasicBlockNo> blockTargets;
    std::vector<VarIndex> conditionalVariables;
    public:
        BB(BasicBlockNo blockno, const std::set<BasicBlockNo>& graphTargets) : blockno(blockno)
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
                conditionalVariables = assignmentOrder;
                std::cout<<"size of conditional variables: "<<conditionalVariables.size()<<std::endl;
                std::cout<<NUM_VARIABLES_IN_CONDITIONAL<<std::endl;
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
                std::cout<<"size of conditional variables: "<<conditionalVariables.size()<<std::endl;
            }
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
                for(int i = 0; i < NUM_VARS; i++)
                {
                    std::string coefficientKey = generateConditionalCoefficientName(blockno, 0, i);
                    parameters[coefficientKey] = std::nullopt;
                }
                std::string constantKey = generateConditionalConstantName(blockno, blockTargets[0]);
                parameters[constantKey] = std::nullopt;
            }
        }
        std::string generateCode() const
        {
            std::ostringstream code;

            // Generate the label for the basic block
            code << generateLabelName(blockno) << ":\n";
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
                code << "    printf(\"";
                for (int i = 0; i < NUM_VARS; ++i) {
                    code << "%d";
                    if (i < NUM_VARS - 1) {
                        code << ",";
                    }
                }
                code << "\", ";
                for (int i = 0; i < NUM_VARS; ++i) {
                    code << getVarName(i);
                    if (i < NUM_VARS - 1) {
                        code << ", ";
                    }
                }
                code << ");\n";
                code << "    return 0;\n";
            }
            return code.str();
        }
        void generateConstraints(int target,  z3::solver& solver, z3::context& c, std::unordered_map<std::string, int>& versions)
        {
            //check if all subexpressions are free of Undefined Behaviour
            int statementIndex = 0;
            for (const auto& [varIndex, dependencies] : statementMappings)
            {
                std::vector<z3::expr> terms;
                // Define integer variables and coefficients
                std::vector<z3::expr> vars;
                std::vector<z3::expr> coeffs;
                for (int i = 0; i < dependencies.size(); i++)
                {
                    vars.push_back(c.int_const((getVarName(dependencies[i]) + "_" + std::to_string(versions[getVarName(dependencies[i])])).c_str()));
                    coeffs.push_back(c.int_const((getCoefficientName(blockno, statementIndex, i)).c_str()));
                    parameters[getCoefficientName(blockno, statementIndex, i)] = std::nullopt;
                    terms.push_back(coeffs.back() * vars.back()); // a_i * var_i
                }
                terms.push_back(c.int_const((generateConstantName(blockno, statementIndex)).c_str()));
                parameters[generateConstantName(blockno, statementIndex)] = std::nullopt;
                coeffs.push_back(c.int_const((generateConstantName(blockno, statementIndex)).c_str()));
                z3::expr sum = terms[0];
                z3::expr term_constraint = (terms[0] <= UPPER_BOUND) && (terms[0] >= LOWER_BOUND);
                if(enableSafetyChecks) solver.add(term_constraint);
                for (int i = 1; i < terms.size(); i++) {
                    z3::expr constraint = (sum <= UPPER_BOUND) && (sum >= LOWER_BOUND);
                    if(enableSafetyChecks) solver.add(constraint);
                    sum = sum + terms[i];
                    term_constraint = (terms[i] <= UPPER_BOUND) && (terms[i] >= LOWER_BOUND);
                    if(enableSafetyChecks) solver.add(term_constraint);
                }
                z3::expr constraint = (sum <= UPPER_BOUND) && (sum >= LOWER_BOUND);
                if(enableSafetyChecks) solver.add(constraint);
                versions[getVarName(varIndex)]++;
                z3::expr assignment = c.int_const((getVarName(varIndex) + "_" + std::to_string(versions[getVarName(varIndex)])).c_str());
                constraint = (assignment == sum);
                solver.add(constraint);
                solver.add(makeCoefficientsInteresting(coeffs, c));
                statementIndex++;
            }
            //now, we need to generate the conditional constraint
            // std::cout<<blockTargets.size()<<std::endl;
            // std::cout<<"Target: "<<target<<std::endl;
            if(blockTargets.size() > 1)
            {
                std::vector<z3::expr> terms;
                // Define integer variables and coefficients
                std::vector<z3::expr> vars;
                std::vector<z3::expr> coeffs;
                int ctr = 0;
                for(auto i: conditionalVariables)
                {
                    vars.push_back(c.int_const((getVarName(i) + "_" + std::to_string(versions[getVarName(i)])).c_str()));
                    coeffs.push_back(c.int_const((generateConditionalCoefficientName(blockno, 0, ctr)).c_str()));
                    parameters[generateConditionalCoefficientName(blockno, 0, ctr)] = std::nullopt;
                    terms.push_back(coeffs.back() * vars.back()); 
                    ++ctr;
                }
                solver.add(makeCoefficientsInteresting(coeffs, c));
                terms.push_back(c.int_const((generateConditionalConstantName(blockno, blockTargets[0])).c_str()));
                parameters[generateConditionalConstantName(blockno, blockTargets[0])] = std::nullopt;
                coeffs.push_back(c.int_const((generateConditionalConstantName(blockno, blockTargets[0])).c_str()));
                z3::expr sum = terms[0];
                z3::expr term_constraint = (terms[0] <= UPPER_BOUND) && (terms[0] >= LOWER_BOUND);
                if(enableSafetyChecks) solver.add(term_constraint);
                for (int i = 1; i < terms.size(); i++) {
                    z3::expr constraint = (sum <= UPPER_BOUND) && (sum >= LOWER_BOUND);
                    if(enableSafetyChecks) solver.add(constraint);
                    sum = sum + terms[i];
                    term_constraint = (terms[i] <= UPPER_BOUND) && (terms[i] >= LOWER_BOUND);
                    if(enableSafetyChecks) solver.add(term_constraint);
                }
                z3::expr constraint = (sum <= UPPER_BOUND) && (sum >= LOWER_BOUND);
                if(enableSafetyChecks) solver.add(constraint);
                if(target == blockTargets[0])
                {
                    constraint = (sum >= 0);
                }
                else
                {
                    constraint = (sum < 0);
                }
                solver.add(constraint);
            }
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
                    std::cout << "Parameter " << name << " is in the model with value: " 
                              << int_value << std::endl;
                }
            }
        } else {
            std::cout << "Parameter " << name << " is not explicitly defined in the model\n";
        }
    }

}
void dumpVariableDefinitions(const std::string& filename, z3::model &model, z3::context &ctx) {
    std::ofstream outputFile(filename);

    // Add necessary includes
    outputFile << "#include <stdio.h>\n\n";

    // Declare and define variables
    for (int i = 0; i < NUM_VARS; ++i) {
        std::string varName = "var_" + std::to_string(i) + "_0";
        outputFile << "int var_" << i << " = ";

        // Query the Z3 model for the variable's value
        z3::expr varExpr = ctx.int_const(varName.c_str());
        if (model.has_interp(varExpr.decl())) {
            z3::expr value = model.get_const_interp(varExpr.decl());
            if (value.is_numeral()) {
                int intValue;
                if (Z3_get_numeral_int(ctx, value, &intValue)) {
                    outputFile << intValue << "; // Initial value from Z3 model\n";
                } else {
                    outputFile << "0; // Default value (failed to extract value)\n";
                }
            } else {
                outputFile << "0; // Default value (non-numeral value)\n";
            }
        } else {
            outputFile << "0; // Default value (not in model)\n";
        }
    }

    outputFile.close();
    std::cout << "Variable definitions have been written to '" << filename << "'." << std::endl;
}
void dumpChronologicalValuesToCSV(const std::string& filename, z3::model &model, z3::context &ctx, std::unordered_map<std::string, int> &versions) {
    std::ofstream outputFile(filename);
    // Query the Z3 model for the values of the variables in chronological order
    for (int i = 0; i < NUM_VARS; ++i) {
        int version = 0;
        try{
            version = versions.at("var_" + std::to_string(i));
        }
        catch(const std::out_of_range& e)
        {
            version = 0;
        }
        std::string varName = "var_" + std::to_string(i) + "_" + std::to_string(version);

        // Query the Z3 model for the variable's value
        z3::expr varExpr = ctx.int_const(varName.c_str());
        if (model.has_interp(varExpr.decl())) {
            z3::expr value = model.get_const_interp(varExpr.decl());
            if (value.is_numeral()) {
                int intValue;
                if (Z3_get_numeral_int(ctx, value, &intValue)) {
                    outputFile << intValue; // Write the value to the CSV
                } else {
                    outputFile << "ERROR"; // Failed to extract value
                }
            } else {
                outputFile << "NON_NUMERAL"; // Non-numeral value
            }
        } else {
            outputFile << "NOT_IN_MODEL"; // Variable not in the model
        }

        // Add a comma if it's not the last variable
        if (i < NUM_VARS - 1) {
            outputFile << ",";
        }
    }
    outputFile.close();
    std::cout << "Chronological values have been written to '" << filename << "'." << std::endl;
}
void generateStaticallyResolvableCode(const std::string& filename, const std::vector<BB>& basicBlocks, const std::unordered_map<std::string, int>& versions, z3::model &model, z3::context &ctx) {
    std::ofstream outputFile(filename);

    // Add necessary includes
    outputFile << "#include <stdio.h>\n\n";

    // Declare and define variables
    for (int i = 0; i < NUM_VARS; ++i) {
        std::string varName = "var_" + std::to_string(i) + "_0";
        outputFile << "static int var_" << i << " = ";

        // Query the Z3 model for the variable's value
        z3::expr varExpr = ctx.int_const(varName.c_str());
        if (model.has_interp(varExpr.decl())) {
            z3::expr value = model.get_const_interp(varExpr.decl());
            if (value.is_numeral()) {
                int intValue;
                if (Z3_get_numeral_int(ctx, value, &intValue)) {
                    outputFile << intValue << "; // Initial value from Z3 model\n";
                } else {
                    outputFile << "0; // Default value (failed to extract value)\n";
                }
            } else {
                outputFile << "0; // Default value (non-numeral value)\n";
            }
        } else {
            outputFile << "0; // Default value (not in model)\n";
        }
    }
    outputFile << "\n";
    outputFile << "int main() {\n";
    // Generate the code for each basic block
    for (const auto& bb : basicBlocks) {
        outputFile << bb.generateCode() << std::endl;
    }
    outputFile << "}\n";
    outputFile.close();
    std::cout << "Statically resolvable code has been written to '" << filename << "'." << std::endl;
}
void generateErrorCode(const std::string& filename, const std::vector<BB>& basicBlocks, const std::unordered_map<std::string, int>& versions, z3::context &ctx) {
    std::ofstream outputFile(filename);

    // Add necessary includes
    outputFile << "#include <stdio.h>\n\n";

    // Declare and define variables
    for (int i = 0; i < NUM_VARS; ++i) {
        std::string varName = "var_" + std::to_string(i) + "_0";
        outputFile << "static int var_" << i << " = 0;\n"; // Default value

        // Query the Z3 model for the variable's value
    }
    outputFile << "\n";
    outputFile << "int main() {\n";
    // Generate the code for each basic block
    for (const auto& bb : basicBlocks) {
        outputFile << bb.generateCode() << std::endl;
    }
    outputFile << "}\n";
    outputFile.close();
    std::cout << "Statically resolvable code has been written to '" << filename << "'." << std::endl;
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
        std::cout << "Please provide a sample number as a command line argument." << std::endl;
        return 1;
    }
    z3::set_param("parallel.enable", true);

    int  nodes = NODES; // Number of nodes
    Graph g(nodes);
    g.generate_graph();

    std::vector<std::set<int>> adjacency_list = g.adj;
    std::vector<int> sample_walk = {};
    if(enableConsistentWalks)
    {
        sample_walk = g.sample_consistent_walk(0, nodes - 1, 100);
    }
    else
    {
        sample_walk = g.sample_walk(0, nodes - 1, 100);
    }
    g.print_graph();
    std::vector<BB> basicBlocks;
    for(int i = 0; i < nodes; i++)
    {
        BB bb(i, adjacency_list[i]);
        basicBlocks.push_back(bb);
    }
    z3::context c;
    z3::solver solver(c);
    solver.add(makeInitialisationsInteresting(c));
    std::unordered_map<std::string, int> versions;
    for(int i = 0; i < sample_walk.size() - 1; i++)
    {
        int current_bb = sample_walk[i];
        int next_bb = sample_walk[i + 1];
        basicBlocks[current_bb].generateConstraints(next_bb, solver, c, versions);
    }
    basicBlocks[sample_walk[sample_walk.size() - 1]].generateConstraints(-1, solver, c, versions);
    //dump solver query yo another file
    std::ofstream outputSMTFile("solver_query_basic.smt2");
    outputSMTFile << solver.to_smt2();
    outputSMTFile.close();
    if (solver.check() == z3::sat) {
        std::cout << "SATISFIABLE" << std::endl;
        z3::model m = solver.get_model();
        extractParametersFromModel(m, c);
        //print all version numbrs in the map versions to the console
        std::cout << "Versions:\n";
        for (const auto& version : versions) {
            std::cout << version.first << ": " << version.second << std::endl;
        }
        dumpVariableDefinitions("definitions/variable_definitions_" + std::to_string(sample_number) + ".c", m, c);
        dumpChronologicalValuesToCSV("gold/chronological_values_" + std::to_string(sample_number) + ".csv", m, c, versions);
        generateStaticallyResolvableCode("inlinable/stat_resolvable_" + std::to_string(sample_number) + ".c", basicBlocks, versions, m, c);

        std::cout << "Model:\n" << m << std::endl;
    } else {
        std::cout << "UNSATISFIABLE" << std::endl;

        // std::cout << "Solver output:\n" << solver.to_smt2() << std::endl;
        //output the generated code to a file
        generateErrorCode("error/error_code_" + std::to_string(sample_number) + ".c", basicBlocks, versions, c);
        std::cout << "Generated code has been written to 'stat_resolvable.c'." << std::endl;
        exit(1);
    }
    z3::expr_vector assertions = solver.assertions();
    std::cout << "Method 1 - Z3 assertions:\n" << assertions << std::endl;
    for(auto x: sample_walk)
    {
        std::cout<<x<<", ";
    }
    std::ofstream outputFile("main_code/generated_code_" + std::to_string(sample_number) + ".c");
    outputFile << "#include <stdio.h>\n\n";
    for (int i = 0; i < NUM_VARS; ++i)
    {
        outputFile << "extern int var_" << i << ";\n";
    }
    outputFile << "\n";
    outputFile << "int main() {\n";

    for (auto bb : basicBlocks) {
        outputFile << bb.generateCode() << std::endl;
    }
    outputFile << "}\n";
    outputFile.close();
    // Dump the generated code to a file
    std::cout << "C code has been generated and written to 'generated_code.c'." << std::endl;
    return 0;
}