package raid.hack.crypto.fantom;


import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;
import raid.hack.crypto.fantom.response.CompileResponse;
import raid.hack.crypto.fantom.response.ExecutionResponse;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

@RestController
public class CompilationAPI {
  private static final String[] SUPPORTED_FORMATS = {
      "c", "cpp"
  };

  @PostMapping("/api/compile")
  public CompileResponse compile(@RequestParam(value = "code") String code,
                                 @RequestParam(value = "format") String format,
                                 @RequestParam(value = "smart", required = false) String smart) {
    if (Arrays.stream(SUPPORTED_FORMATS).noneMatch(format::equals)) {
      return null;
    }

    String id = CoderHelper.nextIdentifier();
    if ("smart_contract".equals(smart)) {
      byte[] mainLoopData = new byte[0];
      try {
        mainLoopData = getClass().getClassLoader().getResourceAsStream("smart_loop.cpp").readAllBytes();
        code = code + new String(mainLoopData, StandardCharsets.UTF_8);
      } catch (IOException exc) {
        exc.printStackTrace();
        return null;
      }
    }
    writeFile(id, format, code);

    ExecutionResponse llvmResp = compileToLlvm(id, format);
    ExecutionResponse transResp = translateToRbvm(id);
    ExecutionResponse disasmResp = disassemble(id);

    String result = readResult(id);
    cleanUp(id);

    return new CompileResponse(result, llvmResp, transResp, disasmResp);
  }

  private void cleanUp(String fileId) {
    new File(fileId).delete();
    new File(fileId + ".c").delete();
    new File(fileId + ".cpp").delete();
    new File(fileId + ".ll").delete();
    new File(fileId + ".rbvm").delete();
  }

  private void writeFile(String fileId, String format, String code) {
    try (var writer = new BufferedWriter(new FileWriter(fileId + "." + format))) {
      writer.write(code);
    } catch (IOException exc) {
      exc.printStackTrace();
    }
  }

  private ExecutionResponse compileToLlvm(String fileId, String format) {
    String compiler = format.equals("c") ? "clang-7" : format.equals("cpp") ? "clang++-7" : null;
    try {
      return
          ExecutionController
              .runAndJoin(compiler, "-S", "-emit-llvm", "-DJUDGE", fileId + "." + format, "-o", fileId + ".ll");
    } catch (InterruptedException exc) {
      return null;
    }
  }

  private ExecutionResponse translateToRbvm(String fileId) {
    try {
      return ExecutionController.runAndJoin("bin/llvm-rbvm", fileId + ".ll");
    } catch (InterruptedException exc) {
      return null;
    }
  }

  private ExecutionResponse disassemble(String fileId) {
    try {
      return ExecutionController.runAndJoin("bin/da", fileId + ".rbvm");
    } catch (InterruptedException exc) {
      return null;
    }
  }

  private String readResult(String fileId) {
    byte[] bytes = readFile(fileId + ".rbvm");
    if (bytes == null)
      return null;

    return CoderHelper.toHexSentence(bytes);
  }

  private byte[] readFile(String fileName) {
    try (InputStream stream = new FileInputStream(fileName)) {
      return stream.readAllBytes();
    } catch (IOException exc) {
      exc.printStackTrace();
      return null;
    }
  }
}
