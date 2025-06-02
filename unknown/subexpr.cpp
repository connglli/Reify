#include <bits/stdc++.h>
#include "better_graph.hpp"
#include"z3++.h"
#include <optional>
#include "params.hpp"
#include <signal.h>
#include <unistd.h>
using VarIndex = int;
using BasicBlockNo = int;
std::ofstream logFile;
class BB;
std::unordered_map<std::string, std::optional<int>> parameters;
static bool isFormulaSatisfiable = false;
z3::expr atMostKZeroes(z3::context& ctx, const std::vector<z3::expr>& vec, int k, int val) {
    z3::expr_vector zero_constraints(ctx);
    
    for (const auto& expr : vec) {
        zero_constraints.push_back(z3::ite(expr == val, ctx.int_val(1), ctx.int_val(0)));
    }
    
    z3::expr sum_zeros = sum(zero_constraints);
    return sum_zeros <= k;
}
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
    //let's assign each variable a random value between -100 and 10
    return atMostKZeroes(c, allVars, NUM_VARS/2, 0);
}
z3::expr arbitrarilyMutateVariable(z3::expr var)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    //multiply by a random number
    std::uniform_int_distribution<int> bitShiftDist(0, 1);
    int bitShift = bitShiftDist(gen);
    z3::expr mutatedVar = var;
    if(bitShift)
    {
        std::uniform_int_distribution<int> bitShiftAmountDist(0, BITSHIFT_BY);
        int bitShiftAmount = bitShiftAmountDist(gen);
        mutatedVar = mutatedVar << BITSHIFT_BY;
    }
    std::uniform_int_distribution<int> dist(INT64_MIN, INT64_MAX);
    int randomNum = dist(gen);
    z3::expr randomNumExpr = var.ctx().int_val(randomNum);
    z3::expr mutatedVar = mutatedVar * randomNumExpr;    
    return mutatedVar;
}
z3::expr generateCrazyExpression(z3::context& c)
{
    //let's choose a random number of variables
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, NUM_VARS);
    int numVars = dist(gen);
    z3::expr finalAssignmentStatement = c.int_val(0);
    for(int i = 0; i < numVars; i++)
    {
        std::uniform_int_distribution<int> varDist(0, NUM_VARS - 1);
        int varIndex = varDist(gen);
        finalAssignmentStatement = finalAssignmentStatement + arbitrarilyMutateVariable(c.int_const((getVarName(varIndex).c_str())));
        //now do we want to have this final expresssion be modulo a certain prime?
        std::uniform_int_distribution<int> moduloDist(0, 1);
        int modulo = moduloDist(gen);
        if(modulo)
        {
            std::uniform_int_distribution<int> primeDist(0, NUM_PRIMES - 1);
            int primeIndex = primeDist(gen);
            finalAssignmentStatement = finalAssignmentStatement % MODULO_PRIME;
        }
    }
}
class BB{
    public:
    BasicBlockNo blockno;
    int numStatements;
    std::map<VarIndex, std::vector<VarIndex>> statementMappings;
    std::map<VarIndex, z3::expr> assignmentMappingsExpr;
    std::vector<VarIndex> assignmentOrder;
    std::vector<BasicBlockNo> blockTargets;
    std::vector<VarIndex> conditionalVariables;
    bool isBlockDead = false;
    public:
        BB(BasicBlockNo blockno, const std::set<BasicBlockNo>& graphTargets, bool isDead = false) : blockno(blockno), isBlockDead(isDead)
        {
            if(isDead)
            {
                //we can go crazy and generate a random number of statements
                numStatements = std::rand() % 10 + 1;
                assignmentOrder = sampleKDistinct(NUM_VARS, numStatements);
                int statementIndex = 0;
                for (const auto& varIndex : assignmentOrder)
                {
                    //for each varIndex to be assigned to, let's generate an expression
                    z3::expr randomExpression = generateCrazyExpression();
                    assignmentMappingsExpr[varIndex] = randomExpression; 
                }
            }
            else
            {
                //
            }
            
        }
        std::string generateCode() const
        {
            if(isDead)
            {
                //just output all the assignment statements in the form of newline separated statements
                std::ostringstream code;
                // Generate the label for the basic block
                code << generateLabelName(blockno) << ":\n";
                // code<< "    printf(\"starting off at "<<generateLabelName(blockno)<<"\\n\");\n";
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<int> dist(LOWER_BOUND, UPPER_BOUND);
                for (const auto& [varIndex, expression] : assignmentMappingsExpr)
                {
                    code << "    " << getVarName(varIndex) << " = ";
                    code << expression;
                    code << ";\n";
                }
            }
            else
            {
                //
            }

        }
        void generateConstraints(int target,  z3::solver& solver, z3::context& c, std::unordered_map<std::string, int>& versions)
        {
            assert(!isBlockDead);
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

z3::expr generateBoundBitwidthConstraints(z3::expr leafNode) {
    assert(leafNode.is_const() || leafNode.is_var());
    z3::expr constraint = (leafNode <= UPPER_BOUND) && (leafNode >= LOWER_BOUND);
    return constraint;
}
z3::expr generateSafetyConstraintsForExpression(z3::context& c, z3::expr expression) {
    z3::expr_vector constraints(c);
    if (expression.is_const() || expression.is_var()) {
        std::cout << expression << " is a const or var" << std::endl;
        constraints.push_back(generateBoundBitwidthConstraints(expression));
    }
    else if (expression.is_app()) {
        for (unsigned i = 0; i < expression.num_args(); ++i) {
            constraints.push_back(generateSafetyConstraintsForExpression(c, expression.arg(i)));
        }
        //now let's generate operator specific constraints
        std::cout<<expression.decl().name() << " is the operator" << std::endl;
        std::cout<<expression.decl().decl_kind() << " is the operator kind" << std::endl;
        switch (expression.decl().decl_kind()) {
            case Z3_OP_ADD:
                // For addition, we need to ensure the result is within bounds
                assert(expression.num_args() == 2);
                //addition of two expressions should be within bounds
                std::cout << "pushing addition constraints" << std::endl;
                constraints.push_back(generateBoundBitwidthConstraints(expression));
                break;
            case Z3_OP_SUB:
                // For subtraction, we need to ensure the result is within bounds
                assert(expression.num_args() == 2);
                std::cout << "pushing subtraction constraints" << std::endl;
                constraints.push_back(generateBoundBitwidthConstraints(expression));
                break;
            case Z3_OP_MUL:
                // For multiplication, we need to ensure the result is within bounds
                assert(expression.num_args() == 2);
                std::cout << "pushing multiplication constraints" << std::endl;
                constraints.push_back(generateBoundBitwidthConstraints(expression));
                break;
            case Z3_OP_BV2INT:
                // std::cout << "Bit-vector to integer conversion operator" << std::endl;
                // assert(expression.num_args() == 2);
                // // For bit-vector to integer conversion, we need to ensure the result is within bounds
                // constraints.push_back(generateBoundBitwidthConstraints(expression));
                break;
            case Z3_OP_INT2BV:
                // std::cout << "Integer to bit-vector conversion operator" << std::endl;
                // assert(expression.num_args() == 2);
                // // For integer to bit-vector conversion, we need to ensure the result is within bounds
                // constraints.push_back(generateBoundBitwidthConstraints(expression.arg(0)));
                break;
            case  Z3_OP_BSHL  : {
                std::cout<<"Pushing bitwise shift left constraints"<<std::endl;
                std::cout << "Bitwise shift left operator" << std::endl;
                std::cout << "Left shift operator" << std::endl;
                // For left shift, we need to ensure the result is within bounds, and that the shift amount is valid
                std::cout << expression.arg(0) << " is the left operand" << std::endl;
                std::cout << expression.arg(1) << " is the right operand" << std::endl;
                // left shift of two expressions should be within bounds
                z3::expr left_operand_as_signed_integer = expression.arg(0).arg(0);
                int bitwidth = expression.arg(0).get_sort().bv_size();
                z3::expr right_operand_as_signed_integer = expression.arg(1).arg(0);
                z3::expr max_value_of_left_operand = c.int_val((long)(((long)(1)) << (long)(bitwidth -1)) - 1);
                //since this is the bitshift of a signed integer, we need to ensure that the shift amount is valid
                std::cout << expression.arg(1) << " is the shift amount" << std::endl;
                constraints.push_back(right_operand_as_signed_integer >= 0 && right_operand_as_signed_integer < bitwidth);
                //also, the first argument should be non-negative
                constraints.push_back(left_operand_as_signed_integer >= 0); 
                //and following the C semantics for left shift, the left shifted result should fit within the unsigned bit-width of the first argument
                constraints.push_back(left_operand_as_signed_integer <= z3::bv2int(z3::lshr(z3::int2bv(bitwidth, max_value_of_left_operand), expression.arg(1)), true));
                break;
            }
            // case Z3_OP_BV
            default:
                assert(false && "Unsupported operator");
                break;
        }
    }
    return z3::mk_and(constraints);
    
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
    logFile << "Variable definitions have been written to '" << filename << "'." << '\n';
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
    logFile << "Chronological values have been written to '" << filename << "'." << '\n';
}
void generateStaticallyResolvableCode(const std::string& filename, const std::vector<BB>& basicBlocks, const std::unordered_map<std::string, int>& versions, z3::model &model, z3::context &ctx, bool statMod = true) {
    std::ofstream outputFile(filename);

    // Add necessary includes
    outputFile << "#include <stdio.h>\n\n";

    // Declare and define variables
    for (int i = 0; i < NUM_VARS; ++i) {
        std::string varName = "var_" + std::to_string(i) + "_0";
        if(statMod)
        {
            outputFile << "static ";
        }
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
    outputFile << "\n";
    outputFile << "int main() {\n";
    // Generate the code for each basic block
    for (const auto& bb : basicBlocks) {
        outputFile << bb.generateCode() << '\n';
    }
    outputFile << "}\n";
    outputFile.close();
    logFile << "Statically resolvable code has been written to '" << filename << "'." << '\n';
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
        outputFile << bb.generateCode() << '\n';
    }
    outputFile << "}\n";
    outputFile.close();
}

void generateMainCode(std::vector<BB> basicBlocks, const std::string& filename, z3::model &model, z3::context &ctx)
{
    // std::ofstream outputFile("main_code/generated_code_" + std::to_string(sample_number) + ".c");
    std::ofstream outputFile(filename);
    outputFile << "#include <stdio.h>\n\n";
    for (int i = 0; i < NUM_VARS; ++i)
    {
        outputFile << "extern int var_" << i << ";\n";
    }
    outputFile << "\n";
    outputFile << "int main() {\n";

    for (auto bb : basicBlocks) {
        outputFile << bb.generateCode() << '\n';
    }
    outputFile << "}\n";
    outputFile.close();

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
int main()
{
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
    std::vector<int> sample_walk = {};
    if(enableConsistentWalks)
    {
        sample_walk = g.sample_consistent_walk(0, nodes - 1, 100);
    }
    else
    {
        sample_walk = g.sample_walk(0, nodes - 1, 100);
    }
    // g.print_graph();
    std::vector<BB> basicBlocks;
    for(int i = 0; i < nodes; i++)
    {
        BB bb(i, adjacency_list[i], true);
        basicBlocks.push_back(bb);
    }
    z3::context c;
    z3::solver solver(c);
    solver.add(makeInitialisationsInteresting(c));
    std::unordered_map<std::string, int> versions;
    sample_walk.clear();
    for(int i = 0; i < sample_walk.size() - 1; i++)
    {
        int current_bb = sample_walk[i];
        int next_bb = sample_walk[i + 1];
        basicBlocks[current_bb].generateConstraints(next_bb, solver, c, versions);
    }
    // basicBlocks[sample_walk[sample_walk.size() - 1]].generateConstraints(-1, solver, c, versions);
    //dump solver query yo another file
    //dump graph and basic blocks to a file
    std::ofstream graphFile("graphs/graph_" + std::to_string(sample_number) + ".txt");
    for (int i = 0; i < nodes; i++) {
        graphFile << i << ": ";
        for (const auto& target : adjacency_list[i]) {
            graphFile << target << ",";
        }
        graphFile << '\n';
    }
    graphFile.close();
    logFile << "Graph has been written to 'graph_" + std::to_string(sample_number) + ".txt'." << '\n';
    //write sample_walk to a file
    std::ofstream sampleWalkFile("walks/sample_walk_" + std::to_string(sample_number) + ".txt");
    for (const auto& node : sample_walk) {
        sampleWalkFile << node << ",";
    }
    sampleWalkFile << '\n';
    sampleWalkFile.close();
    std::ofstream basicBlocksFile("basic_blocks/basic_blocks_" + std::to_string(sample_number) + ".txt");
    logFile <<"Sample walk has been written to 'sample_walk_" + std::to_string(sample_number) + ".txt'." << '\n';
    basicBlocksFile << "Basic Blocks "<< '\n';
    for (const auto& bb : basicBlocks) {
        basicBlocksFile << bb.blockno << ": ";
        int statementIndex = 0;
        for (const auto& [varIndex, dependencies] : bb.statementMappings) {
            basicBlocksFile<<varIndex << " = ";
            for (size_t i = 0; i < dependencies.size(); ++i) {
                basicBlocksFile << getCoefficientName(bb.blockno, statementIndex, i) << " * " << getVarName(dependencies[i]);
                if (i < dependencies.size() - 1) {
                    basicBlocksFile << ","; // Example operation
                }
            }
            ++statementIndex;
            basicBlocksFile << "," << generateConstantName(bb.blockno, statementIndex) << ";" << '\n';
        }
        basicBlocksFile << "Conditional Variables: ";
        for (const auto& var : bb.conditionalVariables) {
            basicBlocksFile << var << ",";
        }
        basicBlocksFile << '\n';
        basicBlocksFile << "Targets: ";
        for (const auto& target : bb.blockTargets) {
            basicBlocksFile<< target << ",";
        }
        basicBlocksFile << '\n';
        basicBlocksFile << "Code: " << bb.generateCode() << '\n';
        basicBlocksFile << '\n'; 
    }
    basicBlocksFile.close();
    logFile << "Basic blocks have been written to 'basic_blocks_" + std::to_string(sample_number) + ".txt'." << '\n';
    std::ofstream outputSMTFile("smt_queries/solver_query_basic_" + std::to_string(sample_number) + ".smt2");
    outputSMTFile << solver.to_smt2();
    outputSMTFile.close();
    if (solver.check() == z3::sat) {
        
        isFormulaSatisfiable = true;
        logFile << "SATISFIABLE" << '\n';
        // usleep(50000000);
        z3::model m = solver.get_model();
        extractParametersFromModel(m, c);
        //print all version numbrs in the map versions to the console
        logFile << "Versions:\n";
        for (const auto& version : versions) {
            logFile << version.first << ": " << version.second << '\n';
        }
        dumpVariableDefinitions("definitions/variable_definitions_" + std::to_string(sample_number) + ".c", m, c);
        generateMainCode(basicBlocks, "main_code/generated_code_" + std::to_string(sample_number) + ".c", m, c);
        dumpChronologicalValuesToCSV("gold/chronological_values_" + std::to_string(sample_number) + ".csv", m, c, versions);
        generateStaticallyResolvableCode("static_inlinable/stat_resolvable_" + std::to_string(sample_number) + ".c", basicBlocks, versions, m, c, true);
        generateStaticallyResolvableCode("inlinable/stat_resolvable_" + std::to_string(sample_number) + ".c", basicBlocks, versions, m, c, false);
        //dump model to a file
        std::ofstream modelFile("models/model_" + std::to_string(sample_number) + ".txt");
        modelFile << m << '\n';
        modelFile.close();
        logFile << "Model has been written to 'model_" + std::to_string(sample_number) + ".txt'." << '\n';
    } else {
        logFile << "UNSATISFIABLE" << '\n';
        generateErrorCode("error/error_code_" + std::to_string(sample_number) + ".c", basicBlocks, versions, c);
        logFile << "Generated code has been written to 'stat_resolvable.c'." << '\n';
        exit(1);
    }
    // z3::expr_vector assertions = solver.assertions();
    // logFile << "Method 1 - Z3 assertions:\n" << assertions << '\n';
    for(auto x: sample_walk)
    {
        logFile << x<<", ";
    }
    // Dump the generated code to a file
    logFile << "C code has been generated and written to 'generated_code.c'." << '\n';
    logFile.close();
    return 0;
}