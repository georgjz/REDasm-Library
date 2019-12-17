// customasm.h

#include "../../plugins/plugins.h" // Import AssemblerPlugin
#include "../../support/dispatcher.h"

class Mos6502Assembler: public AssemblerPlugin
{
    public:
      Mos6502Assembler();
      virtual const char* name() const;                                      // Assembler's display name

    protected
      virtual bool decodeInstruction(const BufferView& view const InstructionPtr& instruction); // Our decoding function
      virtual void onDecoded(const InstructionPtr& instruction) const; // ...when instruction is decoded...

    private:
      Dispatcher<u16, void(Buffer&, const InstructionPtr&)> m_decodemap; // Our decoding map
};

DECLARE_ASSEMBLER_PLUGIN(Mos6502Assembler, mos6502) // Declare the plugin with id: CustomAssembler=customasm