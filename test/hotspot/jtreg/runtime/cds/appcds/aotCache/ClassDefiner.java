import java.util.function.Consumer;

public class ClassDefiner implements Consumer<byte[]> {
    public void accept(byte[] buff) {
        try {
            new Shadower().defineClass(buff);
        } catch (IllegalAccessException e) {
            throw new RuntimeException(e);
        }
    }    

}
