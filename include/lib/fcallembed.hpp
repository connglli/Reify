#ifndef REIFY_FCALLEMBED_HPP
#define REIFY_FCALLEMBED_HPP

#include "lib/lang.hpp"
#include "lib/argument.hpp"


class FCallStrategy {
public: 
	void initialize(
		const symir::Funct *guest,
		const std::vector<ArgPlus<int>> *init,
		const std::vector<ArgPlus<int>> *fina
	);
	void setTarget(const int coefVal);
		
	/// generates the nessessary preamble that create the function arguments
	virtual std::vector<symir::Block *> generatePreamble() = 0;
	/// generates the nessessary postamble that map the function call's return value back to the replaced coeff
	virtual std::vector<symir::Block *> generatePostamble() = 0;
	/// generate the string representing the call
	virtual std::string generateCall() = 0;

protected:
	const symir::Funct *guest;
	const std::vector<ArgPlus<int>> *init;
	const std::vector<ArgPlus<int>> *fina;
	int coefVal;
};

class FCallEmbedder : protected symir::SymIRVisitor {
public:
	explicit FCallEmbedder(symir::Funct *const host);

	void setStrategy(FCallStrategy *callGenStrategy) {
		Assert(
			callGenStrategy != nullptr,
			"The callGenStrategy passed to the setStragegy method is a nullptr"
		);
		this->callGenStrategy = callGenStrategy;
	}

	/// embeds the guest function with a coeff
	bool embedGuest(
		symir::Funct *guest,
		const std::vector<ArgPlus<int>> *init,
		const std::vector<ArgPlus<int>> *fina
	);

protected:
	symir::Funct *const host;
	FCallStrategy *callGenStrategy;
	std::map<symir::Coef *, bool> symbols;
	bool succeeded = false;
	bool wasMutated(symir::Coef *c);
	void markMutated(symir::Coef *c);
};

class LiteralFCallStrategy : FCallStrategy {
	std::vector<symir::Block *> generatePreamble() override;
	std::vector<symir::Block *> generatePostamble() override;
	std::string generateCall() override;
};

class RandomFCallEmbedder : FCallEmbedder {
	void Visit(const symir::VarUse &v) override;
	void Visit(const symir::Coef &c) override;
	void Visit(const symir::Term &t) override;
	void Visit(const symir::Expr &e) override;
	void Visit(const symir::Cond &c) override;
	void Visit(const symir::AssStmt &a) override;
	void Visit(const symir::RetStmt &r) override;
	void Visit(const symir::Branch &b) override;
	void Visit(const symir::Goto &g) override;
	void Visit(const symir::ScaParam &p) override;
	void Visit(const symir::VecParam &p) override;
	void Visit(const symir::StructParam &p) override;
	void Visit(const symir::ScaLocal &l) override;
	void Visit(const symir::VecLocal &l) override;
	void Visit(const symir::StructLocal &l) override;
	void Visit(const symir::StructDef &s) override;
	void Visit(const symir::Block &b) override;
	void Visit(const symir::Funct &f) override;
};

#endif // REIFY_FCALLEMBED_HPP
