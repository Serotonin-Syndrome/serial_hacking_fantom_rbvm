package raid.hack.crypto.fantom;

import java.util.Arrays;
import java.util.Random;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

public class CoderHelper {
  private static final int ID_LENGTH = 8;
  private static final String ALPHABET = IntStream.concat(
      IntStream.rangeClosed('a', 'z'), IntStream.rangeClosed('A', 'Z'))
      .mapToObj(x -> String.valueOf((char) x))
      .collect(Collectors.joining());
  private static final Random random = new Random();

  public synchronized static String nextIdentifier() {
    return IntStream.generate(() -> random.nextInt(ALPHABET.length()))
        .limit(ID_LENGTH)
        .mapToObj(i -> String.valueOf(ALPHABET.charAt(i)))
        .collect(Collectors.joining());
  }

  public static String toHexSentence(byte[] bytes) {
    StringBuilder builder = new StringBuilder();
    for (int i = 0; i < bytes.length; i++) {
      String hex = Integer.toHexString(Byte.toUnsignedInt(bytes[i]));
      builder.append("0x");
      if (hex.length() < 2)
        builder.append('0');
      builder.append(hex);
      if (i != bytes.length - 1)
        builder.append(' ');
    }

    return builder.toString();
  }

  public static byte[] fromHexSentence(String sentence) {
    int[] data = Arrays.stream(sentence.split("\\s+"))
        .peek(hex -> {
          if (hex.length() != 4 || !hex.startsWith("0x"))
            throw new RuntimeException("Incorrect codes");
        })
        .map(hex -> hex.substring(2))
        .mapToInt(hex -> Integer.parseInt(hex, 16))
        .toArray();

    byte[] bytes = new byte[data.length];
    for (int i = 0; i < bytes.length; i++)
      bytes[i] = (byte) data[i];
    return bytes;
  }
}
