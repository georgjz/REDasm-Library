#include "algorithm_impl.h"
#include <redasm/disassembler/disassembler.h>
#include <redasm/plugins/assembler/assembler.h>
#include <redasm/plugins/loader/loader.h>
#include <redasm/support/utils.h>
#include <redasm/context.h>

#define INVALID_MNEMONIC "db"
#define REGISTER_STATE_PRIVATE(id, cb) m_states[id] = std::bind(cb, q, std::placeholders::_1)

namespace REDasm {

AlgorithmImpl::AlgorithmImpl(Algorithm *algorithm, Disassembler *disassembler): StateMachine(), m_pimpl_q(algorithm), m_document(disassembler->document()), m_disassembler(disassembler), m_assembler(disassembler->assembler()), m_currentsegment(nullptr), m_analyzed(0)
{
    m_analyzer = nullptr;
    m_loader = m_disassembler->loader();

    //if(assembler->hasFlag(AssemblerFlags::CanEmulate))
        //m_emulator = std::unique_ptr<Emulator>(assembler->createEmulator(disassembler));

    PIMPL_Q(Algorithm);

    REGISTER_STATE_PRIVATE(Algorithm::DecodeState, &Algorithm::decodeState);
    REGISTER_STATE_PRIVATE(Algorithm::JumpState, &Algorithm::jumpState);
    REGISTER_STATE_PRIVATE(Algorithm::CallState, &Algorithm::callState);
    REGISTER_STATE_PRIVATE(Algorithm::BranchState, &Algorithm::branchState);
    REGISTER_STATE_PRIVATE(Algorithm::BranchMemoryState, &Algorithm::branchMemoryState);
    REGISTER_STATE_PRIVATE(Algorithm::AddressTableState, &Algorithm::addressTableState);
    REGISTER_STATE_PRIVATE(Algorithm::MemoryState, &Algorithm::memoryState);
    REGISTER_STATE_PRIVATE(Algorithm::PointerState, &Algorithm::pointerState);
    REGISTER_STATE_PRIVATE(Algorithm::ImmediateState, &Algorithm::immediateState);
}

size_t AlgorithmImpl::disassembleInstruction(address_t address, const InstructionPtr &instruction)
{
    if(!this->canBeDisassembled(address))
        return Algorithm::SKIP;

    Symbol* symbol = m_document->symbol(address);

    if(symbol && !symbol->isLocked() && !symbol->is(SymbolType::Code))
        m_document->eraseSymbol(symbol->address);

    instruction->address = address;

    BufferView view = m_loader->view(address);
    return m_assembler->decode(view, instruction) ? Algorithm::OK : Algorithm::FAIL;
}

void AlgorithmImpl::done(address_t address) { m_done.insert(address); }
void AlgorithmImpl::enqueue(address_t address) { DECODE_STATE(address);  }

void AlgorithmImpl::analyze()
{
    if(m_analyzed)
    {
        r_ctx->status("Analyzing (Fast)...");
        m_analyzer->analyzeFast();
        m_disassembler->computeBasicBlocks();
        m_document->moveToEP();
        return;
    }

    m_analyzed = true;
    Loader* loader = m_disassembler->loader();
    m_analyzer = loader->analyzer(m_disassembler);

    r_ctx->status("Analyzing...");
    m_analyzer->analyze();
    m_disassembler->computeBasicBlocks();
    m_document->moveToEP();

    // Trigger a Fast Analysis when post disassembling is completed
    EVENT_CONNECT(m_disassembler, busyChanged, this, [&]() {
        if(m_disassembler->busy())
            return;

        this->analyze();
    });
}

void AlgorithmImpl::loadTargets(const InstructionPtr &instruction)
{
    for(address_t target : instruction->meta.targets) // Get precalculated targets
        m_disassembler->pushTarget(target, instruction->address);
}

bool AlgorithmImpl::validateState(const State &state) const
{
    if(!StateMachine::validateState(state))
        return false;

    return m_document->segment(state.address);
}

void AlgorithmImpl::validateTarget(const InstructionPtr &instruction) const
{
    if(m_disassembler->getTargetsCount(instruction->address))
        return;

    const Operand* op = instruction->target();

    if(op && !op->isNumeric())
        return;

    r_ctx->problem("No targets found for " + Utils::quoted(instruction->mnemonic) + " @ " + Utils::hex(instruction->address));
}

bool AlgorithmImpl::canBeDisassembled(address_t address)
{
    BufferView view = m_loader->view(address);

    if(view.eob())
        return false;

    if(!m_currentsegment || !m_currentsegment->contains(address))
        m_currentsegment = m_document->segment(address);

    if(!m_currentsegment || !m_currentsegment->is(SegmentType::Code))
        return false;

    if(!m_loader->offset(address).valid)
        return false;

    return true;
}

void AlgorithmImpl::createInvalidInstruction(const InstructionPtr &instruction)
{
    if(!instruction->size)
        instruction->size = 1; // Invalid instruction uses at least 1 byte

    instruction->type = InstructionType::Invalid;
    instruction->mnemonic = INVALID_MNEMONIC;
}

size_t AlgorithmImpl::disassemble(address_t address, const InstructionPtr &instruction)
{
    auto it = m_done.find(address);

    if(it != m_done.end())
        return Algorithm::SKIP;

    this->done(address);
    size_t result = this->disassembleInstruction(address, instruction);

    PIMPL_Q(Algorithm);

    if(result == Algorithm::FAIL)
    {
        this->createInvalidInstruction(instruction);
        q->onDecodeFailed(instruction);
    }
    else
    {
        this->emulate(instruction);
        q->onDecoded(instruction);
    }

    return result;
}

void AlgorithmImpl::emulateOperand(const Operand *op, const InstructionPtr &instruction)
{
    // u64 value = 0;

    // if(op->is(OperandType::Register))
    // {
    //     if(!m_emulator->read(op, &value))
    //         return;
    // }
    // else if(op->is(OperandType::Displacement))
    // {
    //     if(!m_emulator->displacement(op, &value))
    //         return;
    // }
    // else
    //     return;

    // this->onEmulatedOperand(op, instruction, value);
}

void AlgorithmImpl::emulate(const InstructionPtr &instruction)
{
    //if(!m_emulator)
        //return;

    //m_emulator->emulate(instruction);
}

} // namespace REDasm