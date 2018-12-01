package raid.hack.crypto.fantom.response;

public class MaintainExecutionResponse {
  private final String maintainId;
  private final ExecutionResponse executionResponse;

  public MaintainExecutionResponse(String maintainId, ExecutionResponse executionResponse) {
    this.maintainId = maintainId;
    this.executionResponse = executionResponse;
  }

  public String getMaintainId() {
    return maintainId;
  }

  public ExecutionResponse getExecutionResponse() {
    return executionResponse;
  }
}
