import java.lang.invoke.MethodHandles;
import java.util.function.Consumer;

public class Shadower implements Consumer<byte[]>{
    public static Class<?> defineClass(byte[] buff) throws IllegalAccessException {
        Class<?> c = MethodHandles.lookup().defineClass(buff);
        System.out.println("Shadower: Defined " + c + " in " + c.getClassLoader());
        return c;
    }
    public void accept(byte[] buff) {
        try {
            defineClass(buff);
        } catch (IllegalAccessException e) {
            throw new RuntimeException(e);
        }
    }    
}
