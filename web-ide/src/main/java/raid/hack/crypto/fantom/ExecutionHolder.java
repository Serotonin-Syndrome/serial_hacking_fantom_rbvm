package raid.hack.crypto.fantom;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class ExecutionHolder {
  private static final long TIMEOUT = 30*60*1000; // 30 minutes

  public static final ExecutionHolder instance = new ExecutionHolder();
  private ExecutionHolder() {}

  private Map<String, ExecutionController> controllers = new ConcurrentHashMap<>();
  private Map<String, Long> lastCallTime = new ConcurrentHashMap<>();

  public String run(String... commands) {
    ExecutionController controller = new ExecutionController(commands);
    String id = CoderHelper.nextIdentifier();
    controllers.put(id, controller);
    controller.onTermination(proc -> controllers.remove(id));
    lastCallTime.put(id, System.currentTimeMillis());

    cleanUnused();
    return id;
  }

  public ExecutionController get(String id) {
    ExecutionController controller = controllers.get(id);
    if (controller != null)
      lastCallTime.put(id, System.currentTimeMillis());

    cleanUnused();
    return controller;
  }

  public void cleanUnused() {
    long currentTime = System.currentTimeMillis();
    lastCallTime.forEach((id, time) -> {
      if (currentTime - time > TIMEOUT)
        controllers.computeIfPresent(id, (ids, controller) -> {
          controller.terminate(true);
          return controller;
        });
    });
  }
}
