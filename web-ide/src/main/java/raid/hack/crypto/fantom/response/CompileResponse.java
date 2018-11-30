package raid.hack.crypto.fantom.response;

public class CompileResponse {
  private final String bytecode;
  private final ExecutionResponse llvmExecution, translatorExecution, disassemblerExecution;

  public CompileResponse(String result,
                         ExecutionResponse llvm, ExecutionResponse translator, ExecutionResponse disassembler) {
    this.bytecode = result;
    this.llvmExecution = llvm;
    this.translatorExecution = translator;
    this.disassemblerExecution = disassembler;
  }

  public String getBytecode() {
    return bytecode;
  }

  public ExecutionResponse getLlvmExecution() {
    return llvmExecution;
  }

  public ExecutionResponse getTranslatorExecution() {
    return translatorExecution;
  }

  public ExecutionResponse getDisassemblerExecution() {
    return disassemblerExecution;
  }
}
