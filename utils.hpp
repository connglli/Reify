
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