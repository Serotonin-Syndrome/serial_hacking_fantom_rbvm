package raid.hack.crypto.fantom;


import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;
import raid.hack.crypto.fantom.response.ExecutionResponse;
import raid.hack.crypto.fantom.response.MaintainExecutionResponse;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.Scanner;
import java.util.logging.Logger;

@RestController
public class RunCodeAPI {
  private static final String RBVM_PATH = "bin/rbvm";

  @PostMapping("/api/run")
  public ExecutionResponse runRbvmBytecode(@RequestParam(value = "bytecode") String code) {
    String id = CoderHelper.nextIdentifier();
    writeFile(id, code);
    ExecutionResponse resp = execute(id);
    cleanUp(id);
    return resp;
  }

  @PostMapping("/api/run-maintain")
  public MaintainExecutionResponse runInMaintainMode(@RequestParam(value = "bytecode") String code) {
    String id = CoderHelper.nextIdentifier();
    writeFile(id, code);
    String maintainId = ExecutionHolder.instance.run("stdbuf", "-oL", RBVM_PATH, id);
    ExecutionController controller = ExecutionHolder.instance.get(maintainId);
    String initOut = controller.openInput().nextLine();
    return new MaintainExecutionResponse(maintainId, new ExecutionResponse(initOut, null));
  }

  @PostMapping("/api/communicate-maintain")
  public ExecutionResponse exchangeLines(@RequestParam(value = "id") String id,
                              @RequestParam(value = "line") String line) {
    ExecutionController controller = ExecutionHolder.instance.get(id);
    if (controller == null)
      return new ExecutionResponse(null, "No such process");

    Scanner scanner = controller.openInput();
    PrintWriter output = controller.openOutput();
    output.println(line);
    output.flush();
    String outLine = scanner.nextLine();
    return new ExecutionResponse(outLine, null);
  }

  private void writeFile(String id, String code) {
    byte[] bytes = CoderHelper.fromHexSentence(code);
    try (BufferedOutputStream stream = new BufferedOutputStream(new FileOutputStream(id))) {
      stream.write(bytes);
    } catch (IOException exc) {
      exc.printStackTrace();
    }
  }

  private void cleanUp(String id) {
    new File(id).delete();
  }

  private ExecutionResponse execute(String id) {
    try {
      return ExecutionController.runAndJoin(RBVM_PATH, id);
    } catch (InterruptedException exc) {
      return null;
    }
  }
}
