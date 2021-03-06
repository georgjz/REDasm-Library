#pragma once

#include "../../plugins/assembler/algorithm/algorithm.h"

namespace REDasm {

class DEXLoader;
struct DEXDebugInfo;
struct DEXEncodedMethod;

class DalvikAlgorithm: public AssemblerAlgorithm
{
    DEFINE_STATES(StringIndexState = UserState, MethodIndexState,
                  PackedSwitchTableState, SparseSwitchTableState, FillArrayDataState,
                  DebugInfoState)

    private:
        typedef std::unordered_map<address_t, std::list<s32> > PackagedCaseMap;
        typedef std::unordered_map<u64, address_t> SparseCaseMap;

    public:
        DalvikAlgorithm(DisassemblerAPI* disassembler, AssemblerPlugin* assemblerplugin);

    protected:
        void validateTarget(const InstructionPtr&) const override;
        void onDecodedOperand(const Operand *op, const InstructionPtr& instruction) override;
        void onDecoded(const InstructionPtr& instruction) override;
        void decodeState(const State *state) override;

    private:
        void stringIndexState(const State* state);
        void methodIndexState(const State* state);
        void packedSwitchTableState(const State* state);
        void sparseSwitchTableState(const State* state);
        void fillArrayDataState(const State* state);
        void debugInfoState(const State* state);
        void emitCaseInfo(address_t address, const PackagedCaseMap& casemap);
        void emitCaseInfo(address_t address, const InstructionPtr &instruction, const SparseCaseMap& casemap);
        void emitArguments(const State* state, const DEXEncodedMethod &dexmethod, const DEXDebugInfo &dexdebuginfo);
        void emitDebugData(const DEXDebugInfo &dexdebuginfo);
        void checkImport(const State *state);
        bool canContinue(const InstructionPtr& instruction);

    private:
        DEXLoader* m_dexloader;
        std::unordered_set<std::string> m_imports;
        std::unordered_set<address_t> m_methodbounds;
};

} // namespace REDasm
