#include "lib/chksum.hpp"
#include "lib/lang.hpp"
#include <sstream>
#include <string>
#include "lib/fcallembed.hpp"
#include <lib/random.hpp>

// ==================== FCallStrategy Base Implementations ====================
void FCallStrategy::initialize(
	const symir::Funct *guest,
	const std::vector<ArgPlus<int>> *init,
	const std::vector<ArgPlus<int>> *fina
) {
	this->guest = guest;
	this->init = init;
	this->fina = fina;
}

void FCallStrategy::setTarget(const int coefVal) {
	this->coefVal = coefVal;
}

// ==================== FCallEmbedder Base Implementations ====================
FCallEmbedder::FCallEmbedder(symir::Funct *const host): host(host) {
	Assert(
		host != nullptr,
		"The host function passed to the constructur is a nullptr"
	);
	for (auto &sym: host->GetSymbols()) {
		// Check to ensure all symbols are Coef
		Assert(
			sym->IsSolved(),
			"The symbol \"%s\" in the host function is not solved, cannot replace it",
			sym->GetName().c_str()
		);
		this->symbols[dynamic_cast<symir::Coef *>(sym)] = false;
	}
}

bool FCallEmbedder::embedGuest(
	symir::Funct *guest,
	const std::vector<ArgPlus<int>> *init,
	const std::vector<ArgPlus<int>> *fina
) {
	this->callGenStrategy->initialize(guest, init, fina);
	this->succeeded = false;
	this->host->Accept(*this);
	return this->succeeded;
}

bool FCallEmbedder::wasMutated(symir::Coef *c) {
	auto it = symbols.find(c);
	Assert(
			it != symbols.end(),
			"The coefficient with name \"%s\" is not found in the host function's symbols",
			it->first->GetName().c_str()
	);
	return it->second;
}
void FCallEmbedder::markMutated(symir::Coef *c) {
	auto it = symbols.find(c);
	Assert(
			it != symbols.end(),
			"The coefficient with name \"%s\" is not found in the host function's symbols",
			it->first->GetName().c_str()
	);
	it->second = true;
}

// ==================== LiteralFCallStrategy Implementations ====================
std::vector<symir::Block *> LiteralFCallStrategy::generatePreamble() {
	// Literal strategy has no preamble
	return {};
}
std::vector<symir::Block *> LiteralFCallStrategy::generatePostamble() {
	// Literal strategy has no postamble
	return {};
}
std::string LiteralFCallStrategy::generateCall() {
	Assert(
		this->guest && this->init && this->fina && this->coefVal,
		"FCallStrategy is not properly initialized"
	);

	int32_t checksum = StatelessChecksum::Compute(*this->fina);
	std::ostringstream fcall;
	fcall << StatelessChecksum::GetCheckChksumName()
				<< "(" 
				<< checksum 
				<< ", "
				<< this->guest->GetName() 
				<< "(";

	const auto &params = this->guest->GetParams();
	for (int i = 0; i < static_cast<int>(init->size()); ++i) {
		const auto &p = params[i];
		const auto &arg = (*init)[i];
		fcall << arg.GetTypeCastStr(p) 
					<< arg.ToCxStr();
		if (i < static_cast<int>(init->size()) - 1) {
			fcall << ", ";
		}
	}
	fcall << "))";
	// To avoid UBs, we'd use an upper type to save the result: long long here
	long long diff = static_cast<long long>(this->coefVal)
								 - static_cast<long long>(checksum);
	if (
			diff >= static_cast<long long>(INT32_MIN) 
			&& diff <= static_cast<long long>(INT32_MAX)
		) {
		return "(" + fcall.str() + " + " + std::to_string(diff) + ")";
	} else {
		return "(int) ((long long)" + fcall.str() + " + " + std::to_string(diff) + "L)";
	}
}

// ==================== RandomFCallEmbedder Implementations ====================
void RandomFCallEmbedder::Visit(const symir::VarUse &v) {
	if (this->succeeded) {
		return;
	}
	if (v.IsVector()) {
		for (const auto *c: v.GetAccess()) {
			c->Accept(*this);
			if (this->succeeded) {
				return;
			}
		}
	}
 }
void RandomFCallEmbedder::Visit(const symir::Coef &c) {
	if (this->succeeded) {
		return;
	}
	auto *pc = const_cast<symir::Coef *>(&c);
	if (wasMutated(pc)) {
		// If the coefficient is already replaced, we do not need to do anything
		return;
	}
  // Replace the coefficient with a call to the function
  Assert(
    pc->GetType() == symir::SymIR::I32,
    "Unsupported type %s for the coefficient with name \"%s\"",
    symir::SymIR::GetTypeName(pc->GetType()).c_str(), pc->GetName().c_str()
  );

	this->callGenStrategy->setTarget(pc->GetI32Value());
	// TODO: handle post and preamble;
	pc->SetValue(this->callGenStrategy->generateCall());
	markMutated(pc);
	this->succeeded = true;
 }

 void RandomFCallEmbedder::Visit(const symir::Term &t) {
	if (this->succeeded) {
		return;
	}
	t.GetCoef()->Accept(*this);
	if (this->succeeded) {
		return;
	}
	if (t.GetVar() != nullptr) {
		t.GetVar()->Accept(*this);
	}
}

void RandomFCallEmbedder::Visit(const symir::Expr &e) {
	if (this->succeeded) {
		return;
	}
	for (const auto *term: e.GetTerms()) {
		if (this->succeeded) {
			return;
		}
		term->Accept(*this);
	}
}

void RandomFCallEmbedder::Visit(const symir::Cond &c) {
 if (this->succeeded) {
 	return;
 }
 c.GetExpr()->Accept(*this);
}
void RandomFCallEmbedder::Visit(const symir::AssStmt &a) {
	if (this->succeeded) {
		return;
	}
	a.GetExpr()->Accept(*this);
	if (this->succeeded) {
		return;
	}
	a.GetVar()->Accept(*this);
 }
 void RandomFCallEmbedder::Visit(const symir::RetStmt &r) { /* Do Nothing */ }
 void RandomFCallEmbedder::Visit(const symir::Branch &b) {
	if (this->succeeded) {
		return;
	}
	b.GetCond()->Accept(*this);
 }
void RandomFCallEmbedder::Visit(const symir::Goto &g)        { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::ScaParam &p)    { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::VecParam &p)    { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::StructParam &p) { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::ScaLocal &l)    { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::VecLocal &l)    { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::StructLocal &l) { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::StructDef &s)   { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::Block &b) {
	Panic("Cannot reach here: We directly manipulate statements when visiting a function");
}
void RandomFCallEmbedder::Visit(const symir::Funct &f) {
	const auto statements = f.GetStmts();
	const auto rand = Random::Get().Uniform(0, static_cast<int>(statements.size()) - 1);
	const auto probability = Random::Get().UniformReal()();
	// Sample a statement from the list of statements for replacement
	for (int tries = 0; tries < 1000; tries++) {
		const int index = rand();
		const auto &stmt = statements[index];
		// Idea: 80% of the replacements should be in the conditional statements,
		// and the rest 20% in those assignment statements.
		double threshold;
		switch (stmt->GetIRId()) {
			case symir::SymIR::SIR_TGT_BRA:
				threshold = 0.8;
				break;
			case symir::SymIR::SIR_STMT_ASS:
				threshold = 0.2;
				break;
			default:
				threshold = 0;
				break;
		}
		if (probability < threshold) {
			stmt->Accept(*this);
			if (this->succeeded) {
				return;
			}
		}
	}
}
