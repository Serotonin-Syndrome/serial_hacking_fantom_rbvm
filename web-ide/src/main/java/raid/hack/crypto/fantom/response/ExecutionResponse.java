package raid.hack.crypto.fantom.response;

public class ExecutionResponse {
  private final String stdout, stderr;

  public ExecutionResponse(String stdout, String stderr) {
    this.stdout = stdout;
    this.stderr = stderr;
  }

  public String getStdout() {
    return stdout;
  }

  public String getStderr() {
    return stderr;
  }
}
