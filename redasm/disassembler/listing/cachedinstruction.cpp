#include "cachedinstruction.h"
#include <impl/disassembler/listing/cachedinstruction_impl.h>

namespace REDasm {

CachedInstruction::CachedInstruction() { }
CachedInstruction::CachedInstruction(InstructionCache *cache, Instruction *instruction): m_pimpl_p(new CachedInstructionImpl(this, cache, instruction)) { }
Instruction* CachedInstruction::operator->() const { return this->get(); }
REDasm::CachedInstruction::operator bool() const { PIMPL_P(const CachedInstruction); return p; }
size_t CachedInstruction::referenceCount() const { PIMPL_P(const CachedInstruction); return p->referenceCount(); }
Instruction *CachedInstruction::get() const { PIMPL_P(const CachedInstruction); return p->get(); }

} // namespace REDasm