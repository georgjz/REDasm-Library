// customasm.cpp

#include "mos6502.h"

namespace REDasm {

Mos6502Assembler::Mos6502Assembler(): AssemblerPlugin()
{
    /* 
     * Fill m_decodemap with
     * m_decodemap[opcode] = decode_callback;
     */
}

const char* Mos6502Assembler::name() const { return "Mos Technology 6502 Assembler"; }

bool CustomAssembler::decodeInstruction(const BufferView& view, const InstructionPtr& instruction)
{
    u16 opcode = *buffer;    // Read 16-bits from buffer with the right endianness
   
    if(!m_decodemap.contains(opcode))
      return false; // Unknown opcode: REDasm will generate an invalid Instruction

    m_decodemap(buffer, instruction);
    return true;
}

void Mos6502Assembler::onDecoded(const InstructionPtr& instruction) const
{
    // Post decoding code: operand definition, instruction type, etc...
}

}