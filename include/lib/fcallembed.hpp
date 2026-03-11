#ifndef REIFY_FCALLEMBED_HPP
#define REIFY_FCALLEMBED_HPP

#include "lib/lang.hpp"

namespace fcallembed {

	class FCallStrategy {
		public: 
			/// generates the nessessary preamble that create the function arguments
			virtual std::vector<symir::Block *> generatePreamble(/* TODO */);
			/// generates the nessessary postamble that map the function call's return value back to the replaced coeff
			virtual std::vector<symir::Block *> generatePostamble(/* TODO */);
			/// generate the string representing the call
			virtual std::ostringstream generateCall(/* TODO */);
	};
	
	class FCallEmbedder : virtual symir::SymIRVisitor {
		public:
			explicit FCallEmbedder(symir::Funct *const host): host(host) {
				Assert(host != nullptr, "The host function passed to the constructur is a nullptr");
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
	
			void setStrategy(const FCallStrategy *callGenStrategy) {
				Assert(callGenStrategy != nullptr, "The callGenStrategy passed to the setStragegy method is a nullptr");
				this->callGenStrategy = callGenStrategy;
			}
	
			/// embeds the guest function with a coeff
			virtual void embedGuest(symir::Funct *guest, const std::vector<int> &inits, const std::vector<int> &finas);
	
		private:
			symir::Funct *const host;
			const FCallStrategy *callGenStrategy;
			std::map<symir::Coef *, bool> symbols;
	};

}
#endif // REIFY_FCALLEMBED_HPP
