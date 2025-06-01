#include <bits/stdc++.h>
#include "better_graph.hpp"
#include"z3++.h"
#include <optional>
const int NUM_VARS = 10;
const int UPPER_BOUND = INT_MAX;
const int LOWER_BOUND = INT_MIN;
const int ASSIGNMENTS_PER_BB = 3;
const int VARIABLES_PER_ASSIGNMENT_STATEMENT = 3;
const bool enable_safety_checks = true;
std::unordered_map<std::string, std::optional<int>> parameters;
std::map<int, std::vector<int>> basicBlockSampledVars;
//idea: since we don't know exactly what variables will be involved in the conditional block
std::vector<int> sampleKDistinct(int n, int k, int blockno) {
    if(basicBlockSampledVars.find(blockno) = basicBlockSampledVars.end()) return basicBlockSampledVars[blockno];
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
z3::expr makeCoefficientsInteresting(const std::vector<z3::expr>& coeffs, z3::context& c)
{
    z3::expr allZero = c.bool_const(("true"));
    for(auto coeff: coeffs)
    {
        allZero = allZero && (coeff == 0);
    }
    return (!allZero);
}
std::string conditional_guard_to_string(int source, std::set<int> targets)
{

}
std::string generate_conditional_guard(int source, int target, z3::solver& solver, z3::context& c, std::unordered_map<std::string, int>& versions, bool direction = true)
{
    std::vector<z3::expr> terms;
    // Define integer variables and coefficients
    std::vector<z3::expr> vars;
    std::vector<z3::expr> coeffs;
    
    for (int i = 0; i < NUM_VARS; i++) {
        vars.push_back(c.int_const(("var_" + std::to_string(i) + std::string("_") + std::to_string(versions["var_" + std::to_string(i)])).c_str()));
        coeffs.push_back(c.int_const(("c_" + std::to_string(i) + "_" + std::to_string(source) + "_" + std::to_string(target)).c_str()));
        parameters[("c_" + std::to_string(i) + "_" + std::to_string(source) + "_" + std::to_string(target))] = std::nullopt;
        terms.push_back(coeffs.back() * vars.back()); // a_i * var_i
    }
    solver.add(makeCoefficientsInteresting(coeffs, c));
    terms.push_back(c.int_const(("b_if_" + std::to_string(source) + "_" + std::to_string(target)).c_str()));
    coeffs.push_back(c.int_const(("b_if_" + std::to_string(source) + "_" + std::to_string(target)).c_str()));
    parameters[("b_if_" + std::to_string(source) + "_" + std::to_string(target))] = std::nullopt;
    z3::expr sum = terms[0];
    z3::expr term_constraint = (terms[0] <= UPPER_BOUND) && (terms[0] >= LOWER_BOUND);
    if(enable_safety_checks) solver.add(term_constraint);
    for (int i = 1; i < terms.size(); i++) {
        z3::expr constraint = (sum <= UPPER_BOUND) && (sum >= LOWER_BOUND);
        if(enable_safety_checks) solver.add(constraint);
        sum = sum + terms[i];
        term_constraint = (terms[i] <= UPPER_BOUND) && (terms[i] >= LOWER_BOUND);
        if(enable_safety_checks) solver.add(term_constraint);
    }
    z3::expr constraint = (sum <= UPPER_BOUND) && (sum >= LOWER_BOUND);
    if(enable_safety_checks) solver.add(constraint);
    if(direction){constraint = (sum >= 0);} else {constraint = (sum < 0);}
    if(enable_safety_checks) solver.add(constraint);
    return constraint.to_string();
}
std::string generate_assignment_statement(int source, z3::solver& solver, z3::context& c, std::unordered_map<std::string, int>& versions, int varnum)
{
    std::vector<z3::expr> terms;
    // Define integer variables and coefficients
    std::vector<z3::expr> vars;
    std::vector<z3::expr> coeffs;
    for (int i = 0; i< NUM_VARS; i++) 
    {
        vars.push_back(c.int_const(("var_" + std::to_string(i) + "_" + std::to_string(versions["var_" + std::to_string(i)])).c_str()));
        coeffs.push_back(c.int_const(("a_" + std::to_string(i) + "_" + std::to_string(source)).c_str()));
        parameters["a_" + std::to_string(i) + "_" + std::to_string(source)] = std::nullopt;
        terms.push_back(coeffs.back() * vars.back()); // a_i * var_i
    }
    terms.push_back(c.int_const(("b_" + std::to_string(varnum) + "_" + std::to_string(source)).c_str()));
    parameters["b_" + std::to_string(varnum) + "_" + std::to_string(source)] = std::nullopt;
    coeffs.push_back(c.int_const(("b_" + std::to_string(varnum) + "_" + std::to_string(source)).c_str()));
    z3::expr sum = terms[0];
    z3::expr term_constraint = (terms[0] <= UPPER_BOUND) && (terms[0] >= LOWER_BOUND);
    if(enable_safety_checks) solver.add(term_constraint);
    for (int i = 1; i < terms.size(); i++) {
        z3::expr constraint = (sum <= UPPER_BOUND) && (sum >= LOWER_BOUND);
        if(enable_safety_checks) solver.add(constraint);
        sum = sum + terms[i];
        term_constraint = (terms[i] <= UPPER_BOUND) && (terms[i] >= LOWER_BOUND);
        if(enable_safety_checks) solver.add(term_constraint);
    }
    z3::expr constraint = (sum <= UPPER_BOUND) && (sum >= LOWER_BOUND);
    if(enable_safety_checks) solver.add(constraint);
    versions["var_" + std::to_string(varnum)]++;
    z3::expr assignment = c.int_const(("var_" + std::to_string(varnum) + "_" + std::to_string(versions["var_" + std::to_string(varnum)])).c_str());
    constraint = (assignment == sum);
    solver.add(constraint);
    solver.add(makeCoefficientsInteresting(coeffs, c));
    return constraint.to_string();
}
std::vector<std::string> generate_assignment_statements_for_basic_block(int blockno, z3::solver& solver, z3::context& c, std::unordered_map<std::string, int>& versions)
{
    std::vector<std::string> all_assignment_statements = {};
    for (int i = 0; i< NUM_VARS; i++) 
    {
        all_assignment_statements.push_back(generate_assignment_statement(blockno, solver, c, versions, i));
    }
    return all_assignment_statements;
}
void extract_params_from_model(z3::model &m)
{

}
z3::expr makeInitialisationsInteresting(z3::context& c)
{
    z3::expr allZero = c.bool_const(("true"));
    for (int i = 0; i < NUM_VARS; i++) 
    {
        allZero = allZero && (c.int_const(("var_" + std::to_string(i) + "_0").c_str()) == 0);
    }
    return !allZero;
}
int main() {
    int  nodes = 10; // Number of nodes
    Graph g(nodes);
    g.generate_graph();

    std::vector<std::set<int>> adjacency_list = g.adj;
    std::vector<int> sample_walk = g.sample_walk(0, nodes - 1);
    g.print_graph();
    
    std::cout<<std::endl;
    z3::context c;
    z3::solver solver(c);
    solver.add(makeInitialisationsInteresting(c));
    std::unordered_map<std::string, int> versions;
    // auto v = generate_assignment_statements_for_basic_block(0, solver, c, versions);
    for(auto x: sample_walk)
    {
        std::cout<<x<<std::endl;
    }
    for(int i = 0; i < sample_walk.size() - 1; i++)
    {
        int current_bb = sample_walk[i];
        int next_bb = sample_walk[i + 1];
        generate_assignment_statements_for_basic_block(current_bb, solver, c, versions);
        if(g.adj[current_bb].size() == 2){
            generate_conditional_guard(current_bb, next_bb, solver, c, versions, *g.adj[current_bb].begin() ==  next_bb);
        }
    }
    generate_assignment_statements_for_basic_block(sample_walk[sample_walk.size() - 1], solver, c, versions);
    // std::cout<<solver.to_smt2();
    if (solver.check() == z3::sat) {
        std::cout << "SATISFIABLE" << std::endl;
        z3::model m = solver.get_model();
        std::cout << "Model:\n" << m << std::endl;
    } else {
        std::cout << "UNSATISFIABLE" << std::endl;
    }
    for(auto x: sample_walk)
    {
        std::cout<<x<<", ";
    }
    return 0;
}