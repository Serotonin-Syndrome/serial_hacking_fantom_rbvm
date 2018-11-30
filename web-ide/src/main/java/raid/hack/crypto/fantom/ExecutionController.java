package raid.hack.crypto.fantom;


import raid.hack.crypto.fantom.response.ExecutionResponse;

import java.io.IOException;
import java.io.InputStream;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.Scanner;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.function.Consumer;

/**
 * This class serves execution of binaries like translator and VM
 */
public class ExecutionController {
//  private long startTime;
  private ProcessBuilder processBuilder;
  private volatile Process process;
  private volatile CompletableFuture<Process> finish;

  private PrintWriter output;
  private Scanner input;

  private volatile byte[] stdout, stderr;

  public ExecutionController(String... commands) {
    processBuilder = new ProcessBuilder(commands);
    processBuilder.redirectOutput(ProcessBuilder.Redirect.PIPE);
    processBuilder.redirectInput(ProcessBuilder.Redirect.PIPE);
    processBuilder.redirectError(ProcessBuilder.Redirect.PIPE);
    processBuilder.redirectErrorStream(false);
    start();
  }

  private void start() {
    try {
      process = processBuilder.start();
//      startTime = System.currentTimeMillis();
    } catch (IOException exc) {
      exc.printStackTrace();
    }
    finish = process.onExit().thenApply(
        proc -> {
          try {
            stderr = proc.getErrorStream().readAllBytes();
          } catch (IOException exc) {
            exc.printStackTrace();
          }

          if (input == null)
            try {
              stdout = proc.getInputStream().readAllBytes();
            } catch (IOException exc) {
              exc.printStackTrace();
            }
          return proc;
        }
    );
  }

  public synchronized void onTermination(Consumer<Process> action) {
    finish = finish.thenApply(proc -> {
      action.accept(proc);
      return proc;
    });
  }

  public synchronized void terminate(boolean force) {
    if (force)
      process.destroyForcibly();
    else
      process.destroy();
  }

  public void join() throws InterruptedException {
    try {
      finish.get();
    } catch (ExecutionException exc) {
      exc.printStackTrace();
    }
  }

  public byte[] getStderr() {
    return stderr;
  }

  public byte[] getStdout() {
    return stdout;
  }

  public synchronized Scanner openInput() {
    if (input == null) {
      input = new Scanner(process.getInputStream());
    }

    return input;
  }

  public synchronized InputStream getInputStream() {
    return process.getInputStream();
  }

  public synchronized PrintWriter openOutput() {
    if (output == null) {
      output = new PrintWriter(process.getOutputStream());
    }

    return output;
  }


  public static ExecutionResponse runAndJoin(String... params) throws InterruptedException {
    ExecutionController controller = new ExecutionController(params);
    controller.join();
    return new ExecutionResponse(new String(controller.getStdout(), StandardCharsets.UTF_8),
        new String(controller.getStderr(), StandardCharsets.UTF_8));
  }
}
