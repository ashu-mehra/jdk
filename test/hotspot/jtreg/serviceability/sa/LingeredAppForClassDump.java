import java.lang.reflect.Method;
import java.util.Arrays;

import jdk.test.lib.apps.LingeredApp;


// To test for a classfile artifact, add an inner class with the required artifact.
public class LingeredAppForClassDump extends LingeredApp {
    public static void main(String args[]) {
        /* Load all the inner classes */
        Class<?>[] toLoad = new Class[] { NestLvl1.class, NoNestMate.class, ClassWithLongDouble.class };
        Arrays.asList(toLoad).stream().forEach(cls -> {
            try {
                Method doit = cls.getMethod("doit");
                doit.invoke(null);
            } catch (Exception e) {
                throw new RuntimeException("Missing doit() method in class " + cls.getName());
            }
        });
        LingeredApp.main(args);
    }
}

// Class with NestHost attribute
class NestLvl1 {
    public static void doit() {
	System.out.println("NestLvl1");
	NestLvl2.doit();
    }
    class NestLvl2 {
	public static void doit() {
	    System.out.println("NestLvl2");
            NestLvl3.doit();
	}
	class NestLvl3 {
	    public static void doit() {
		System.out.println("NestLvl3");
	    }
	}
    }
}

// Class with no NestHost or NestMember attribute
class NoNestMate {
    public static void doit() {
	System.out.println("NoNestMate");
    }
}

// Class with a long and a double field
class ClassWithLongDouble {
    public static final long LONG_VALUE = Long.MAX_VALUE;
    public static final double DOUBLE_VALUE = Double.MAX_VALUE;

    public static void doit() {
	System.out.println("ClassWithLongDouble");
    }
}
